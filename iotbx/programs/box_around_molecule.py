from __future__ import absolute_import, division, print_function
import os
from libtbx.program_template import ProgramTemplate
#from libtbx.utils import date_and_time
import libtbx.load_env
from cctbx.maptbx.box import shift_and_box_model

master_phil_str = '''
buffer_layer = 5.0
  .type = float
  .help = buffer around atoms, in Angstrom
output {
  suffix = _box
    .type = str
  format = *Auto pdb cif
    .type = choice(multi=False)
    .help = output model file type
}
'''

# ------------------------------------------------------------------------------

class Program(ProgramTemplate):
  description = '''
Script that creates a P1 box around a model.

Inputs:
  PDB or mmCIF file containing atomic model
  optional: buffer layer in Angstrom (default is 5.0 A)

Usage examples:
  1) Save model as PDB (input is mmCIF)
     iotbx.pdb.box_around_molecule.py test.cif format=pdb

  2) Save model as mmCIF (input is PDB)
  pdb.box_around_molecule.py test.pdb format=cif

  The program template accepts PDB and mmCIF input formats.
  The paramter output.format allows choosing the output format.
'''

  datatypes = ['model', 'phil']
  master_phil_str = master_phil_str

  # ----------------------------------------------------------------------------

  def validate(self):
    self.data_manager.has_models(expected_n  = 1,
                                 exact_count = True,
                                 raise_sorry = True)

  # ----------------------------------------------------------------------------

  def run(self):
    #
    self.model = self.data_manager.get_model()

    # shift_and_box_model creates a new model object, so the format needs to
    # be obtained here
    if self.params.output.format == 'Auto':
      if self.model.input_model_format_pdb():
        self.params.output.format = 'pdb'
      elif self.model.input_model_format_cif():
        self.params.output.format = 'cif'
    extension = '.' + self.params.output.format

    self.model = shift_and_box_model(model = self.model,
                                     box_cushion = self.params.buffer_layer)
#    # PDB format
#    if (self.params.output.format == 'pdb'):
#      # If output file should contain REMARK with buffer_layer
#      #model_str = 'REMARK %s --buffer-layer=%.6g %s\n' % (
#      #            libtbx.env.dispatcher_name, self.params.buffer_layer, fn) + \
#      #       'REMARK %s\n' % date_and_time() + \
#      #       self.model.model_as_pdb()
#      print(self.model.model_as_pdb(), file=self.logger)

    # Get output filename if not provided
    fn = self.data_manager.get_default_model_name()
    basename = os.path.splitext(os.path.basename(fn))[0]
    if self.params.output.prefix is None:
      self.params.output.prefix = basename
      self.data_manager.set_default_output_filename(self.get_default_output_filename())

    # save output
    self.data_manager.write_model_file(self.model,
                                       format = self.params.output.format)
    output_fn = self.data_manager.get_default_output_model_filename(
      extension=extension)
    print('Created file', output_fn, file=self.logger)
