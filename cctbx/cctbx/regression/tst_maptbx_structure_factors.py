from cctbx import maptbx
from cctbx import miller
from cctbx import crystal
from cctbx.development import random_structure
from cctbx.development import structure_factor_utils
from cctbx.development import debug_utils
from cctbx.array_family import flex
from scitbx import fftpack
import sys

def exercise_crystal_gridding():
  crystal_symmetry = crystal.symmetry(
    unit_cell=(95.2939, 95.2939, 98.4232, 94.3158, 115.226, 118.822),
    space_group_symbol="Hall: C 2y (x+y,-x+y+z,z)")
  for mandatory_factors,n_real in ((None,(90,90,90)),
                                   ((20,20,20),(100,100,100))):
    crystal_gridding = maptbx.crystal_gridding(
      unit_cell=crystal_symmetry.unit_cell(),
      d_min=3.5,
      resolution_factor=1/3.,
      symmetry_flags=maptbx.use_space_group_symmetry,
      space_group_info=crystal_symmetry.space_group_info(),
      mandatory_factors=mandatory_factors,
      max_prime=5,
      assert_shannon_sampling=True)
    assert crystal_gridding.n_real() == n_real

def exercise_shannon_sampled(space_group_info, anomalous_flag, conjugate_flag,
                             d_min=3., resolution_factor=0.5, max_prime=5,
                             verbose=0):
  structure = random_structure.xray_structure(
    space_group_info,
    elements=("N", "C", "C", "O"),
    random_f_prime_d_min=1,
    random_f_double_prime=anomalous_flag,
    anisotropic_flag=True,
    random_u_iso=True,
    random_occupancy=True)
  f_calc = structure.structure_factors(
    anomalous_flag=anomalous_flag,
    d_min=d_min,
    algorithm="direct").f_calc()
  n_real = f_calc.crystal_gridding(
    resolution_factor=resolution_factor,
    d_min=d_min,
    max_prime=max_prime).n_real()
  if (not anomalous_flag):
    rfft = fftpack.real_to_complex_3d(n_real)
    n_complex = rfft.n_complex()
  else:
    cfft = fftpack.complex_to_complex_3d(n_real)
    n_complex = cfft.n()
  map = maptbx.structure_factors.to_map(
    f_calc.space_group(),
    anomalous_flag,
    f_calc.indices(),
    f_calc.data(),
    n_real,
    flex.grid(n_complex),
    conjugate_flag)
  if (not anomalous_flag):
    real_map = rfft.backward(map.complex_map())
    assert real_map.all() == rfft.m_real()
    complex_map = rfft.forward(real_map)
  else:
    real_map = cfft.backward(map.complex_map())
    assert not real_map.is_padded()
    complex_map = cfft.forward(real_map)
  complex_map /= n_real[0] * n_real[1] * n_real[2]
  assert real_map.focus() == n_real
  assert complex_map.focus() == n_complex
  from_map = maptbx.structure_factors.from_map(
    f_calc.unit_cell(),
    f_calc.space_group_info().type(),
    anomalous_flag,
    d_min,
    complex_map,
    conjugate_flag)
  match = miller.match_indices(
    f_calc.indices(),
    from_map.miller_indices())
  for i in (0,1):
    if (i == 0): m = f_calc.indices()
    else: m = from_map.miller_indices()
    for j in match.singles(i):
      assert abs(f_calc.unit_cell().d(m[j]) - d_min) < 1.e-5
  structure_factor_utils.check_correlation(
    "from_map-1", f_calc.indices(), match,
    f_calc.data(), from_map.data(),
    min_corr_ampl=0.9999, max_mean_w_phase_error=.01,
    verbose=verbose)
  from_map = maptbx.structure_factors.from_map(
    anomalous_flag,
    f_calc.indices(),
    complex_map,
    conjugate_flag)
  assert from_map.miller_indices().size() == 0
  structure_factor_utils.check_correlation(
    "from_map-2", f_calc.indices(), 0,
    f_calc.data(), from_map.data(),
    min_corr_ampl=0.9999, max_mean_w_phase_error=.01,
    verbose=verbose)
  structure_p1 = structure.asymmetric_unit_in_p1()
  miller_p1 = miller.set.expand_to_p1(f_calc)
  f_calc_p1 = miller_p1.structure_factors_from_scatterers(
    xray_structure=structure_p1,
    algorithm="direct").f_calc()
  map = maptbx.structure_factors.to_map(
    f_calc_p1.space_group(),
    anomalous_flag,
    f_calc_p1.indices(),
    f_calc_p1.data(),
    n_real,
    flex.grid(n_complex),
    conjugate_flag)
  from_map = maptbx.structure_factors.from_map(
    f_calc.space_group(),
    anomalous_flag,
    f_calc.indices(),
    map.complex_map(),
    conjugate_flag)
  assert from_map.miller_indices().size() == 0
  assert flex.max(flex.abs(f_calc.data()-from_map.data())) < 1.e-10

def exercise_under_sampled(space_group_info, anomalous_flag, conjugate_flag,
                           under_sampling,
                           d_min=2., resolution_factor=0.5, max_prime=5,
                           verbose=0):
  structure_factors = random_structure.xray_structure(
    space_group_info,
    elements=("N", "C", "C", "O"),
    random_f_prime_d_min=1,
    random_f_double_prime=anomalous_flag,
    anisotropic_flag=True,
    random_u_iso=True,
    random_occupancy=True
    ).structure_factors(
        anomalous_flag=anomalous_flag, d_min=d_min, algorithm="direct")
  f_calc = structure_factors.f_calc()
  n_real = maptbx.crystal_gridding(
    unit_cell=f_calc.unit_cell(),
    d_min=d_min,
    resolution_factor=resolution_factor,
    max_prime=max_prime,
    mandatory_factors=(under_sampling,)*3).n_real()
  if (not anomalous_flag):
    rfft = fftpack.real_to_complex_3d(n_real)
    n_complex = rfft.n_complex()
  else:
    cfft = fftpack.complex_to_complex_3d(n_real)
    n_complex = cfft.n()
  map = maptbx.structure_factors.to_map(
    f_calc.space_group(),
    anomalous_flag,
    f_calc.indices(),
    f_calc.data(),
    n_real,
    flex.grid(n_complex),
    conjugate_flag)
  if (not anomalous_flag):
    real_map = rfft.backward(map.complex_map())
    assert real_map.all() == rfft.m_real()
  else:
    real_map = cfft.backward(map.complex_map())
    assert not real_map.is_padded()
  if (0 or verbose):
    if (not anomalous_flag):
      maptbx.statistics(real_map).show_summary()
      maptbx.statistics(real_map).show_summary()
    else:
      maptbx.statistics(flex.real(real_map)).show_summary()
      maptbx.statistics(flex.imag(real_map)).show_summary()
  n_real_under_sampled = [n/under_sampling for n in n_real]
  if (not anomalous_flag):
    rfft = fftpack.real_to_complex_3d(n_real_under_sampled)
    n_complex_under_sampled = rfft.n_complex()
  else:
    cfft = fftpack.complex_to_complex_3d(n_real_under_sampled)
    n_complex_under_sampled = cfft.n()
  under_sampled_map = maptbx.structure_factors.to_map(
    f_calc.space_group(),
    anomalous_flag,
    f_calc.indices(),
    f_calc.data(),
    n_real_under_sampled,
    flex.grid(n_complex_under_sampled),
    conjugate_flag)
  if (not anomalous_flag):
    under_sampled_map_before_fft = under_sampled_map.complex_map().deep_copy()
    under_sampled_real_map = rfft.backward(under_sampled_map.complex_map())
    assert under_sampled_real_map.all() == rfft.m_real()
  else:
    under_sampled_real_map = cfft.backward(under_sampled_map.complex_map())
    assert not under_sampled_real_map.is_padded()
  if (0 or verbose):
    if (not anomalous_flag):
      maptbx.statistics(under_sampled_real_map).show_summary()
      maptbx.statistics(under_sampled_real_map).show_summary()
    else:
      maptbx.statistics(flex.real(under_sampled_real_map)).show_summary()
      maptbx.statistics(flex.imag(under_sampled_real_map)).show_summary()
  if (0 or verbose):
    print real_map.all(), n_complex
    print under_sampled_real_map.all(), n_complex_under_sampled
  if (not anomalous_flag):
    x_source = real_map
    y_source = under_sampled_real_map
  else:
    x_source = flex.real(real_map)
    y_source = flex.real(under_sampled_real_map)
  x = flex.double()
  n = x_source.focus()
  for i in xrange(0, n[0], under_sampling):
    for j in xrange(0, n[1], under_sampling):
      for k in xrange(0, n[2], under_sampling):
        x.append(x_source[(i,j,k)])
  y = maptbx.copy(y_source, flex.grid(y_source.focus())).as_1d()
  if (0 or verbose):
    print "x:", tuple(x)
    print "y:", tuple(y)
  assert flex.max(flex.abs(x-y)) \
      < (flex.max(flex.abs(x))+flex.max(flex.abs(y)))/2*1.e-6
  if (under_sampling == 1):
    x = maptbx.copy(x_source, flex.grid(x_source.focus())).as_1d()
    c = flex.linear_correlation(x, y)
    assert c.coefficient() >= 0.9999

def run_call_back(flags, space_group_info):
  for anomalous_flag in (False, True)[:]: #SWITCH
    for conjugate_flag in (False, True)[:]: #SWITCH
      for with_shift in [0,1]:
        if (with_shift):
          sgi = debug_utils.random_origin_shift(space_group_info)
        else:
          sgi = space_group_info
      exercise_shannon_sampled(
        space_group_info=sgi,
        anomalous_flag=anomalous_flag,
        conjugate_flag=conjugate_flag,
        verbose=flags.Verbose)
  for anomalous_flag in (False, True)[:]: #SWITCH
    for conjugate_flag in (False, True)[:]: #SWITCH
      for under_sampling in (1,2,3,4,5):
        exercise_under_sampled(space_group_info,
                               anomalous_flag,
                               conjugate_flag,
                               under_sampling,
                               verbose=flags.Verbose)

def run():
  exercise_crystal_gridding()
  debug_utils.parse_options_loop_space_groups(sys.argv[1:], run_call_back)

if (__name__ == "__main__"):
  run()
