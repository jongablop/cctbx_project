from __future__ import absolute_import, division, print_function

# LIBTBX_SET_DISPATCHER_NAME simtbx.diffBragg.stage_one

from libtbx.mpi4py import MPI
from dxtbx.model import ExperimentList
import os
import time
from simtbx.nanoBragg.utils import H5AttributeGeomWriter
from simtbx.diffBragg.utils import image_data_from_expt

COMM = MPI.COMM_WORLD

if COMM.rank > 0:
    import warnings
    warnings.filterwarnings("ignore")

import sys
import json
import numpy as np
from copy import deepcopy

from simtbx.diffBragg.phil import philz
from simtbx.diffBragg import refine_launcher
from dxtbx.model.experiment_list import ExperimentListFactory
from dials.array_family import flex
from libtbx.phil import parse
from dials.util import show_mail_on_error

help_message = "stage 1 (per-shot) diffBragg refinement"

script_phil = """
exper_id = None
  .type = int
  .help = index of an experiment to process
exper_refls_file = None
  .type = str
  .help = path to two column text file specifying exp json files and refls files as pairs
  .help = to be read and processed together
usempi = False
  .type = bool
  .help = process using mpi
squash_errors = False
  .type = bool
  .help = if True, skip expts that fail (useful when processing many expts with mpi)
show_timing = True
  .type = bool
  .help = print a refinement duration for each iteration experiment
reference_from_experiment
  {
  detector = None
    .type = str
    .help = path to experiment containing a reference detector model to use during refinement
  crystal = None
    .type = str
    .help = path to experiment containing a reference crystal model to use during refinement
}
output {
  directory = .
    .type = str
    .help = path where output files and folders will be written
  save {
    model = False
      .type = bool
      .help = if True, save the model in an hdf5 file
    model_and_data = False
      .type = bool
      .help = if True, save the model, the data, and their difference in an hdf5 file
    reflections = False
      .type = bool
      .help = if True, output a refined reflections table with xyz.calc
      .help = computed by the diffBragg model
    pandas = True
      .type = bool
      .help = whether to save a pandas output file
    experiments = True
      .type = bool
      .help = whether to save a refined experiments output file
  }
  tag {
    images = stg1
      .type = str
      .help = output file tag for model images
    reflections = stg1
      .type = str
      .help = output file tag for reflections
    pandas = stg1
      .type = str
      .help = output file tag for pandas
    experiments = stg1
      .type = str
      .help = output file tag for experiments
  }
}
"""

philz = script_phil + philz
phil_scope = parse(philz)


class Script:

    def __init__(self):
        from dials.util.options import OptionParser

        self.parser = None
        if COMM.rank==0:
            self.parser = OptionParser(
                usage="",  # stage 1 (per-shot) diffBragg refinement",
                sort_options=True,
                phil=phil_scope,
                read_experiments=True,
                read_reflections=True,
                check_format=False,
                epilog=help_message)
        self.parser = COMM.bcast(self.parser)

        self.refls = None
        self.explist = None
        self.has_loaded_expers = False
        self.input_expnames, self.input_reflnames, self.input_spectrumnames = [], [], []
        self.panel_strings = []
        self.i_exp = 0

    def _load_exp_refls_fnames(self):
        if COMM.rank == 0:
            with open(self.params.exper_refls_file, "r") as exper_refls_file:
                for line in exper_refls_file:
                    line = line.strip()
                    line_split = line.split()
                    if len(line_split) not in [2, 3, 4]:
                        print("Weird input line %s, continuing" % line)
                        continue
                    if len(line_split) == 2:
                        exp, ref = line_split
                        print(exp, ref)
                    elif len(line_split) == 3:
                        exp, ref, spec_file = line_split
                        self.input_spectrumnames.append(spec_file)
                    else:
                        exp, ref, spec_file, panel_string = line_split
                        self.input_spectrumnames.append(spec_file)
                        self.panel_strings.append(panel_string)

                    self.input_expnames.append(exp)
                    self.input_reflnames.append(ref)
        self.input_expnames = COMM.bcast(self.input_expnames)
        self.input_reflnames = COMM.bcast(self.input_reflnames)
        self.input_spectrumnames = COMM.bcast(self.input_spectrumnames)
        self.panel_strings = COMM.bcast(self.panel_strings)

    def _generate_exp_refl_pairs(self):
        if self.has_loaded_expers:
            for self.i_exp, exper in enumerate(self.explist):
                if self.params.usempi and self.i_exp % COMM.size != COMM.rank:
                    continue
                elif self.params.exper_id is not None and self.params.exper_id != self.i_exp:
                    continue
                refls_for_exper = self.refls.select(self.refls['id'] == self.i_exp)

                # little hack to check the format now
                El = ExperimentList()
                El.append(exper)
                El = ExperimentListFactory.from_dict(El.to_dict())
                if len(self.params.input.experiments)==1:
                    exp_filename = self.params.input.experiments[0].filename + "_%d" % self.i_exp
                else:
                    exp_filename = self.params.input.experiments[self.i_exp].filename
                yield exp_filename, El[0], refls_for_exper

        elif self.params.exper_refls_file is not None:
            self._load_exp_refls_fnames()
            count = 0
            for self.i_exp, (exp_f, refls_f) in enumerate(zip(self.input_expnames, self.input_reflnames)):
                refls = flex.reflection_table.from_file(refls_f)
                nexper_in_refls = len(set(refls['id']))
                if nexper_in_refls > 1 and self.input_spectrumnames:
                    raise NotImplementedError("Cannot input multi-experiment lists and single spectrum files; use dxtbx.beam.spectrum instead")
                for list_i_exp in range(nexper_in_refls):
                    if self.params.usempi and count % COMM.size != COMM.rank:
                        count += 1
                        continue
                    exper = self.exper_json_single_file(exp_f, list_i_exp)
                    refls_for_exper = refls.select(refls['id'] == list_i_exp)
                    count += 1
                    yield exp_f, exper, refls_for_exper

    @staticmethod
    def exper_json_single_file(exp_file, i_exp=0):
        """
        load a single experiment from an exp_file
        If working with large combined experiment files, we only want to load
        one image at a time on each MPI rank, otherwise at least one rank would need to
        load the entire file into memory.
        :param exp_file:
        :param i_exp:
        :return:
        """
        exper_json = json.load(open(exp_file))
        nexper = len(exper_json["experiment"])
        assert 0 <= i_exp < nexper

        this_exper = exper_json["experiment"][i_exp]

        new_json = {'__id__': "ExperimentList", "experiment": [deepcopy(this_exper)]}

        for model in ['beam', 'detector', 'crystal', 'imageset', 'profile', 'scan', 'goniometer', 'scaling_model']:
            if model in this_exper:
                model_index = this_exper[model]
                new_json[model] = [exper_json[model][model_index]]
                new_json["experiment"][0][model] = 0
            else:
                new_json[model] = []
        explist = ExperimentListFactory.from_dict(new_json)
        assert len(explist) == 1
        return explist[0]

    def run(self):
        from dials.util.options import flatten_experiments, flatten_reflections
        self.params = None
        if COMM.rank == 0:
            self.params, _ = self.parser.parse_args(show_diff_phil=True)
        self.params = COMM.bcast(self.params)
        if COMM.size > 1 and not self.params.usempi:
            if COMM.rank == 0:
                print("Using %d MPI ranks, but usempi option is set to False, please try again with usempi=True" % COMM.size)
            sys.exit()

        self.explist = flatten_experiments(self.params.input.experiments)
        reflist = flatten_reflections(self.params.input.reflections)

        if self.explist and self.params.exper_refls_file is not None:
            print("Can't an input-experiments-glob AND exper_refls_file, please to one or the other.")
            sys.exit()

        self.has_loaded_expers = False
        if reflist:
            self.refls = reflist[0]
            for r in reflist[1:]:
                self.refls.extend(r)
            self.has_loaded_expers = True
        if not self.has_loaded_expers and self.params.exper_refls_file is None:
            print("No experiments to process")
            sys.exit()

        i_processed = 0
        for exper_filename, exper, refls_for_exper in self._generate_exp_refl_pairs():
            try:
                if self.input_spectrumnames:
                    self.params.simulator.spectrum.filename = self.input_spectrumnames[self.i_exp]
                if self.panel_strings:
                    self.params.roi.panels = self.panel_strings[self.i_exp]
                    print(self.params.roi.panels)

                assert len(set(refls_for_exper['id'])) == 1

                exp_id = refls_for_exper['id'][0]

                print(exper_filename, COMM.rank, set(refls_for_exper['id']))

                det_ref_exp = self.params.reference_from_experiment.detector
                if det_ref_exp is not None:
                    new_detector = ExperimentListFactory.from_json_file(det_ref_exp, check_format=False)[0].detector
                    assert new_detector is not None
                    assert len(new_detector) == len(exper.detector)
                    exper.detector = new_detector

                cryst_ref_exp = self.params.reference_from_experiment.crystal
                if cryst_ref_exp is not None:
                    new_crystal = ExperimentListFactory.from_json_file(cryst_ref_exp, check_format=False)[0].crystal
                    exper.crystal = new_crystal

                if self.params.output.save.reflections:
                    self.params.refiner.record_xy_calc = True

                refine_starttime = time.time()
                if not self.params.refiner.randomize_devices:
                    self.params.simulator.device_id = COMM.rank % self.params.refiner.num_devices
                # HACK for Gainmap:
                exper.gainmap_path = exper_filename.replace(".expt", "_GAINMAP.expt")
                self.params.refiner.print_end = " -- loggedByRank%d\n" % COMM.rank
                refiner = refine_launcher.local_refiner_from_parameters(refls_for_exper, exper, self.params)
                if self.params.show_timing:
                    print("Time to refine experiment: %f" % (time.time() - refine_starttime))

                basename,_ = os.path.splitext(os.path.basename(exper_filename))

                # Save model image
                if self.params.output.save.model or self.params.output.save.model_and_data:
                    images_outdir = os.path.join(self.params.output.directory, "imgs", "rank%d" % COMM.rank)
                    if not os.path.exists(images_outdir):
                        os.makedirs(images_outdir)
                    img_path = os.path.join(images_outdir, "%s_%s_%d.h5" % (self.params.output.tag.images, basename, i_processed))
                    panel_Xdim, panel_Ydim = exper.detector[0].get_image_size()
                    img_shape = len(exper.detector), panel_Ydim, panel_Xdim
                    num_imgs = 2
                    if self.params.output.save.model_and_data:
                        num_imgs = 6
                    writer_args = {"filename": img_path ,
                                   "image_shape": img_shape,
                                   "num_images": num_imgs,
                                   "detector": exper.detector , "beam": exper.beam}
                    model_img, spots_img, sigma_r_img = refiner.get_model_image()
                    with H5AttributeGeomWriter(**writer_args) as writer:
                        if self.params.output.save.model_and_data:
                            model_img *= self.params.refiner.adu_per_photon
                            data = image_data_from_expt(exper)
                            pids, ys, xs = np.where(model_img == 0)
                            model_img[pids, ys, xs] = data[pids, ys, xs]
                            writer.add_image(model_img)
                            writer.add_image(data)

                            #from skimage import metrics
                            #out = metrics.structural_similarity(model_img,
                            #        data, full=True,
                            #        gaussian_weights=True,
                            #        sigma=0.2, use_sample_covariance=False)
                            #writer.add_image(out[1])

                            Zimg = model_img-data
                            Zimg /= np.sqrt(model_img + sigma_r_img**2)
                            Zimg = Zimg*0.1+1
                            Zimg[pids, ys, xs] = 1

                            Zimg2 = model_img-data
                            Zimg2 /= np.sqrt(data+sigma_r_img**2)
                            Zimg2 = Zimg2*0.1+1
                            Zimg2[pids, ys, xs] = 1

                            writer.add_image(Zimg2)
                            writer.add_image(Zimg)
                            writer.add_image(model_img-data)
                            writer.add_image(spots_img)
                        else:
                            writer.add_image(model_img)
                            writer.add_image(spots_img)

                # Save reflections
                if self.params.output.save.reflections:
                    refined_refls = refiner.get_refined_reflections(refls_for_exper)
                    #NOTE do we really need to reset the id ?
                    refined_refls['id'] = flex.int(len(refined_refls), 0)
                    refls_outdir = os.path.join(self.params.output.directory, "refls", "rank%d" % COMM.rank)
                    if not os.path.exists(refls_outdir):
                        os.makedirs(refls_outdir)
                    refls_path = os.path.join(refls_outdir, "%s_%s_%d.refl" % (self.params.output.tag.reflections, basename, i_processed))
                    refined_refls.as_file(refls_path)

                # save experiment
                if self.params.output.save.experiments:
                    exp_outdir = os.path.join(self.params.output.directory, "expers", "rank%d" % COMM.rank)
                    if not os.path.exists(exp_outdir):
                        os.makedirs(exp_outdir)

                    opt_exp_path = os.path.join(exp_outdir, "%s_%s_%d.expt" % (self.params.output.tag.experiments, basename, exp_id))
                    exper.crystal = refiner.get_corrected_crystal(i_shot=0)
                    exper.detector = refiner.get_optimized_detector()
                    new_exp_list = ExperimentList()
                    new_exp_list.append(exper)
                    new_exp_list.as_file(opt_exp_path)

                # save pandas
                if self.params.output.save.pandas:
                    pandas_outdir = os.path.join(self.params.output.directory, "pandas", "rank%d" % COMM.rank)
                    if not os.path.exists(pandas_outdir):
                        os.makedirs(pandas_outdir)
                    outpath = os.path.join(pandas_outdir, "%s_%s_%d.pkl" % (self.params.output.tag.pandas,basename, i_processed))
                    #TODO add beamsize_mm, mtz_file, mtz_col, pinkstride, oversample, spectrum_file to the pandas dataframe

                    data_frame = refiner.get_lbfgs_x_array_as_dataframe()

                    if self.params.simulator.spectrum.filename is not None:
                        data_frame["spectrum_filename"] = os.path.abspath(self.params.simulator.spectrum.filename)
                        data_frame["spectrum_stride"] = self.params.simulator.spectrum.stride
                    data_frame["total_flux"] = self.params.simulator.total_flux
                    data_frame["beamsize_mm"] = refiner.S.beam.size_mm
                    data_frame["exp_name"] = os.path.abspath(exper_filename)
                    data_frame["oversample"] = self.params.simulator.oversample
                    if self.params.roi.panels is not None:
                        data_frame["roi_panels"] = self.params.roi.panels
                    if self.params.output.save.experiments:
                        data_frame["opt_exp_name"] = os.path.abspath(opt_exp_path)
                    data_frame.to_pickle(outpath)

                i_processed += 1
            except Exception as err:
                print("Encounter exception %s" % err)
                if self.params.squash_errors:
                    i_processed += 1
                    continue
                else:
                    raise RuntimeError("Process failed")


if __name__ == '__main__':
    with show_mail_on_error():
        script = Script()
        script.run()
