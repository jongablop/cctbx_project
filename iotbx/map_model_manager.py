from __future__ import absolute_import, division, print_function
import sys
from libtbx.utils import Sorry
from cctbx import maptbx
from libtbx import group_args
from scitbx.array_family import flex
import iotbx.map_manager
from mmtbx.model import manager as model_manager
import mmtbx.ncs.ncs
from libtbx.utils import null_out
from libtbx.test_utils import approx_equal

class map_model_base(object):
  '''
    Common methods for map_model_manager and r_model
  '''
  def map_dict(self):
    ''' Get the dictionary of all maps and masks as map_manager objects'''
    return self._map_dict

  def crystal_symmetry(self):
    ''' Get the working crystal_symmetry'''
    return self.map_manager().crystal_symmetry()

  def unit_cell_crystal_symmetry(self):
    ''' Get the unit_cell_crystal_symmetry (full or original symmetry)'''
    return self.map_manager().unit_cell_crystal_symmetry()

  def model(self):
    ''' Get the model '''
    return self._model

  def map_managers(self):
    ''' Get all the map_managers as a list'''
    mm_list = []
    for id in self.map_manager_id_list():
      mm_list.append(self.get_map_manager(id))
    return mm_list

  def map_manager(self):
    ''' Get the map_manager '''
    return self._map_dict.get('map_manager')

  def map_manager_1(self):
    ''' Get half_map 1 as a map_manager object '''
    return self._map_dict.get('map_manager_1')

  def map_manager_2(self):
    ''' Get half_map 2 as a map_manager object '''
    return self._map_dict.get('map_manager_2')

  def map_manager_mask(self):
    ''' Get the mask as a map_manager object '''
    return self._map_dict.get('map_manager_mask')

  def map_manager_id_list(self):
    ''' Get all the names (keys) for all map_managers'''
    return list(self.map_dict().keys())

  def get_map_manager(self, map_manager_id):
    ''' Get a map_manager with the name map_manager_id'''
    return self.map_dict().get(map_manager_id)

  def write_map(self, file_name = None, key='map_manager', log = sys.stdout):
    if not self._map_dict.get(key):
      print ("No map to write out with id='%s'" %(key), file = log)
    elif not file_name:
      print ("Need file name to write map", file = log)
    else:
      self._map_dict.get('map_manager').write_map(file_name = file_name)

  def write_model(self,
     file_name = None,
     log = sys.stdout):
    if not self._model:
      print ("No model to write out", file = log)
    elif not file_name:
      print ("Need file name to write model", file = log)
    else:
      # Write out model

      f = open(file_name, 'w')
      print(self._model.model_as_pdb(), file = f)
      f.close()
      print("Wrote model with %s residues to %s" %(
         self._model.get_hierarchy().overall_counts().n_residues,
         file_name), file = log)

  def get_map_manager_info(self):
    '''
      Return a group_args object specifying the map_manager and
      a list of any other maps present
    '''
    all_map_manager_id_list=list(self._map_dict.keys())
    assert all_map_manager_id_list
    all_map_manager_id_list.sort()
    map_manager_id='map_manager'
    other_map_manager_id_list=[]
    for id in all_map_manager_id_list:
      if id != map_manager_id:
        other_map_manager_id_list.append(id)

    return group_args(map_manager_id=map_manager_id,
         other_map_manager_id_list=other_map_manager_id_list)

  def set_map_manager(self, map_manager_id, map_manager):
    ''' Set a map_manager with the name map_manager_id'''
    assert isinstance(map_manager, iotbx.map_manager.map_manager)
    self.map_dict()[map_manager_id] = map_manager

  def initialize_maps(self, map_value = 0):
    '''
      Set values of all maps to map_value
      Used to set up an empty set of maps for filling in from boxes
    '''

    for mm in self.map_managers():
      mm.initialize_map_data(map_value = map_value)

  def not_box_around_model(self,
    cushion,
    selection_string = None,
    soft_mask = False):

   '''
     Select part of model specified by selection_string (all if not present).
     Box this r_model around the selection and return a new r_model object

     Specify cushion = distance of closest atom to edge of box
     Optionally make a soft mask around the edges of the box
   '''

   from cctbx.maptbx.box import around_model
   box = around_model(self.map_manager(),
     self.model(),
     cushion = cushion,
     log = null_out())

   box.map_manager()


  def mask_all_maps_around_atoms(self,
      mask_atoms_atom_radius = 3,
      set_outside_to_mean_inside = False,
      soft_mask = False,
      soft_mask_radius = None,
      mask_key = 'mask'):
    assert mask_atoms_atom_radius is not None
    assert self.model() is not None

    '''
      Generate mask around atoms and apply to all maps.
      Overwrites values in these maps

      Optionally set the value outside the mask equal to the mean inside,
        changing smoothly from actual values inside the mask to the constant
        value outside (otherwise outside everything is set to zero)

      Optional: radius around atoms for masking
      Optional: soft mask  (default = True)
        Radius will be soft_mask_radius
        (default radius is resolution calculated from gridding)
        If soft mask is set, mask_atoms_atom_radius increased by

      Optionally use any mask specified by mask_key
    '''
    if soft_mask:
      if not soft_mask_radius:
        from cctbx.maptbx import d_min_from_map
        soft_mask_radius = d_min_from_map(
          map_data=self.map_manager().map_data(),
          unit_cell=self.map_manager().crystal_symmetry().unit_cell())
      mask_atoms_atom_radius += soft_mask_radius
    self.create_mask_around_atoms(
         soft_mask = soft_mask,
         soft_mask_radius = soft_mask_radius,
         mask_atoms_atom_radius = mask_atoms_atom_radius)
    self.apply_mask_to_maps(mask_key = mask_key,
         set_outside_to_mean_inside = \
           set_outside_to_mean_inside)

  def mask_all_maps_around_edges(self,
      soft_mask_radius = None,
      mask_key = 'mask'):
    '''
      Apply a soft mask around edges of all maps. Overwrites values in maps
      use 'mask' as the mask key
    '''

    self.create_mask_around_edges(soft_mask_radius = soft_mask_radius,
      mask_key = mask_key)
    self.apply_mask_to_maps(mask_key = mask_key)

  def apply_mask_to_map(self,
      map_key,
      mask_key = 'mask',
      set_outside_to_mean_inside = False):
    '''
      Apply the mask in 'mask' to map specified by map_key

      Optionally set the value outside the mask equal to the mean inside,
        changing smoothly from actual values inside the mask to the constant
        value outside (otherwise outside everything is set to zero)

      Optionally use any mask specified by mask_key
    '''

    self.apply_mask_to_maps(map_keys = [map_key],
      mask_key = mask_key,
      set_outside_to_mean_inside = set_outside_to_mean_inside)


  def apply_mask_to_maps(self,
      map_keys = None,
      mask_key = 'mask',
      set_outside_to_mean_inside = False):
    '''
      Apply the mask in 'mask' to maps specified by map_keys.
      If map_keys is None apply to all

      Optionally set the value outside the mask equal to the mean inside,
        changing smoothly from actual values inside the mask to the constant
        value outside (otherwise outside everything is set to zero)

      Optionally use any mask specified by mask_key
    '''

    assert (map_keys is None) or isinstance(map_keys, list)
    # ZZassert isinstance(set_outside_to_mean_inside, bool) # XXX
    mask_mm = self.get_map_manager(mask_key)
    assert mask_mm is not None
    assert mask_mm.is_mask()

    from cctbx.maptbx.segment_and_split_map import apply_mask_to_map

    if map_keys is None:
      map_keys = list(self._map_dict.keys())
    for map_key in map_keys:
      mm=self.get_map_manager(map_key)
      if mm.is_mask(): continue  # don't apply to a mask
      if set_outside_to_mean_inside in [None, True]:
      # smoothly go from actual value inside mask to target value outside
        new_map_data = apply_mask_to_map(mask_data = mask_mm.map_data(),
          set_outside_to_mean_inside = set_outside_to_mean_inside,
          map_data = mm.map_data(),
          out = null_out())
      else: # Simple case  just multiply
        new_map_data = mask_mm.map_data() * mm.map_data()
      # Set the values in this manager
      mm.set_map_data(map_data = new_map_data)

  def create_mask_around_edges(self,
     soft_mask_radius = None,
     mask_key = 'mask' ):
    '''
      Generate soft mask around edges of mask
      Does not apply the mask to anything.

      Optional: radius around edge for masking
        (default radius is resolution calculated from gridding)

      Generates new entry in map_manager dictionary with key of
      mask_key (default='mask') replacing any existing entry with that key
    '''

    if not soft_mask_radius:
      from cctbx.maptbx import d_min_from_map
      soft_mask_radius = d_min_from_map(
         map_data=self.map_manager().map_data(),
         unit_cell=self.map_manager().crystal_symmetry().unit_cell())

    from cctbx.maptbx.mask import create_mask_around_edges
    cm = create_mask_around_edges(map_manager = self.map_manager(),
      soft_mask_radius = soft_mask_radius)
    cm.soft_mask(soft_mask_radius = soft_mask_radius)

    # Put the mask in map_dict keyed with mask_key
    self.set_map_manager(mask_key, cm.map_manager())

  def create_mask_around_atoms(self,
     mask_atoms_atom_radius = 3,
     soft_mask = False,
     soft_mask_radius = None,
     mask_key = 'mask' ):

    '''
      Generate mask based on model.  Does not apply the mask to anything.

      Optional: radius around atoms for masking
      Optional: soft mask  (default = True)
        Radius will be soft_mask_radius
        (default radius is resolution calculated from gridding)
        If soft mask is set, mask_atoms_atom_radius increased by
          soft_mask_radius

      Generates new entry in map_manager dictionary with key of
      mask_key (default='mask') replacing any existing entry with that key
    '''

    if soft_mask:
      if not soft_mask_radius:
        from cctbx.maptbx import d_min_from_map
        soft_mask_radius = d_min_from_map(
           map_data=self.map_manager().map_data(),
           unit_cell=self.map_manager().crystal_symmetry().unit_cell())
      mask_atoms_atom_radius += soft_mask_radius

    from cctbx.maptbx.mask import create_mask_around_atoms
    cm = create_mask_around_atoms(map_manager = self.map_manager(),
      model = self.model(),
      mask_atoms_atom_radius = mask_atoms_atom_radius)

    if soft_mask: # Make the create_mask object contain a soft mask
      cm.soft_mask(soft_mask_radius = soft_mask_radius)

    # Put the mask in map_dict keyed with mask_key
    self.set_map_manager(mask_key, cm.map_manager())

  def create_mask_around_density(self,
     solvent_content = None,
     soft_mask = True,
     soft_mask_radius = None,
     mask_key = 'mask' ):

    '''
      Generate mask based on density in map_manager.
      Does not apply the mask to anything.

      Optional:  supply approximate solvent fraction

      Optional: soft mask  (default = True)
        Radius will be soft_mask_radius
        (default radius is resolution calculated from gridding)

      Generates new entry in map_manager dictionary with key of
      mask_key (default='mask') replacing any existing entry with that key
    '''

    from cctbx.maptbx.mask import create_mask_around_density
    cm = create_mask_around_density(map_manager = self.map_manager(),
        solvent_content = solvent_content)

    if soft_mask: # Make the create_mask object contain a soft mask
      if not soft_mask_radius:
        from cctbx.maptbx import d_min_from_map
        soft_mask_radius = d_min_from_map(
           map_data=self.map_manager().map_data(),
           unit_cell=self.map_manager().crystal_symmetry().unit_cell())
      cm.soft_mask(soft_mask_radius = soft_mask_radius)

    # Put the mask in map_dict keyed with mask_key
    self.set_map_manager(mask_key, cm.map_manager())

  def force_wrapping_if_neccesary(self, map_manager):
    if self._force_wrapping is None:
      return
    else:
      map_manager.set_wrapping(self._force_wrapping)
      if not map_manager.is_full_size():
        self._warning_message = "WARNING: wrapping set to True, but "+\
             "map is not full size"

  def warning_message(self):
    return self._warning_message

class r_model(map_model_base):

  '''
    Class to hold a model, set of maps/mask and optional ncs object and
    perform simple boxing and masking operations

    The model and ncs_object can be accessed with self.model()
      and self.ncs_object()

    There must be a model and a map_manager with the id 'map_manager'

    Any map can be accessed using: self.get_map_manager(map_manager_id)

    For convenience, four special maps can be accessed directly:
      self.map_manager() : the main map
      self.map_manager_1():  half-map 1
      self.map_manager_2():  half_map 2
      self.map_manager_mask():  a mask

    For the four maps above, map_manager_id_list keys are 'map_manager',
       'map_manager_1', 'map_manager_2', and 'map_manager_mask'.

    For all maps, the map_manager_id is the key specified for that map_manager
    in map_dict in the call.

    Map reconstruction symmetry information as a ncs_object is available from
    any of the map_manager objects if it was supplied in the call.

    Notes on boxing and "crystal_symmetry" and "unit_cell_crystal_symmetry":

    The unit_cell_crystal_symmetry is the symmetry and cell dimensions of
    the full box ("unit_cell") corresponding to an original full map.

    The crystal_symmetry is the symmetry and dimensions of the part of the
    map that is present.

    Normally after cutting out a box of density from a map, that box of density
    is shifted to place its origin at (0,0,0).  The shift in grid units to
    put it back is available from a shifted map_manager as
    map_manager.origin_shift_grid_units.

    The shift_cart for a map, model, or ncs_object is the shift that has been
    applied to that object since its original position. This is available from
    any of these objects as e.g., model.shift_cart().  To move it back, apply
    the negative of shift_cart. (This is done automatically on writing any of
    these objects).

    A map or model written out by a boxed version of an r_model object will
     superimpose on the corresponding map or model written out by the
     r_model object that produced it.

    Typical setup is to create a map_model_manager object and then get an
    r_model with map_model_manager.as_r_model().

    Typical uses:
      rm = map_model_manager.as_r_model()  # get r_model

      rm.create_mask_around_atoms()          # create a mask
      rm.apply_mask_to_maps()              # apply the mask


      boxed_rm = rm.box_all_maps_around_model_and_shift_origin(
           selection="chain A") # box with chain A

  '''

  def __init__(self,
               model            = None,
               map_dict         = None,
               wrapping         = None,
      ):

    # Save the model, ncs_object and map_dict (get them with self.model() etc)
    self._map_dict = map_dict
    self._model = model
    self._force_wrapping = wrapping
    self._warning_message = None

    # Make sure all the assumptions about model, ncs_object and map are ok
    self._check_inputs()

    # Ready to go

  def customized_copy(self, model = None, map_dict = None):
    '''
      Produce a copy of this r_model object, replacing maps or model or both
    '''

    # Require that something is new otherwise this is a deep_copy
    assert isinstance(model, model_manager) or isinstance(map_dict, dict)

    if model: # take new model without deep_copy
      new_model = model
    else:  # just keep model
      new_model = self.model().deep_copy()


    if map_dict: # take new map_dict without deep_copy
      new_map_dict = map_dict
    else:  # deep_copy existing map_dict
      new_map_dict = {}
      for id in self.map_manager_id_list():
        new_map_dict[id]=self.get_map_manager(id).deep_copy()

    return r_model(model = new_model, map_dict = new_map_dict,
      wrapping = self._force_wrapping)

  def deep_copy(self):
    '''
      Produce a deep copy of this r_model object
    '''
    new_model = self.model().deep_copy()
    new_map_dict = {}
    for id in self.map_manager_id_list():
      new_map_dict[id]=self.get_map_manager(id).deep_copy()

    return r_model(model = new_model, map_dict = new_map_dict,
      wrapping = self._force_wrapping)

  def __repr__(self):
    text = "r_model: "
    if self.model():
      text += "\n%s" %(str(self.model()))
    info = self.get_map_manager_info()
    if self.map_manager():
      text += "\nmap_manager: %s" %(str(self.map_manager()))
    for id in info.other_map_manager_id_list:
      text += "\n%s: %s" %(id,str(self.get_map_manager(id)))
    return text

  def _check_inputs(self):
    '''
     Make sure that model, map and ncs objects are correct types
     Make sure that all have same crystal_symmetry, unit_cell_crystal_symmetry
     Make sure all are shifted to place origin at (0, 0, 0,0 and
       all have the same shift_cart()
    '''

    # Checks

    model=self.model()
    map_dict=self.map_dict()

    assert isinstance(model, model_manager)
    assert isinstance(map_dict, dict)
    assert map_dict != {}
    assert 'map_manager' in map_dict.keys()

    # Make sure map_manager is already shifted to (0, 0, 0)

    map_manager=map_dict['map_manager']
    assert isinstance(map_manager, iotbx.map_manager.map_manager)
    assert map_manager.origin_is_zero()

    # Make sure all other maps and model are similar
    for id in map_dict.keys():
      m=map_dict[id]
      assert m.origin_is_zero()
      assert m.is_compatible_model(model)
      assert m.is_similar(map_manager)

    # Make sure model and map all have same shift_cart and
    #  crystal_symmetry and unit_cell_crystal_symmetry
    assert map_manager.is_compatible_model(model)

    # All OK


class map_model_manager(map_model_base):

  '''
    Class for shifting origin of map(s) and model to (0, 0, 0) and keeping
    track of the shifts.

    Typical use:
    mam = map_model_manager(
      model = model,
      map_manager = map_manager,
      ncs_object = ncs_object)

    mam.box_all_maps_around_model_and_shift_origin(
        box_cushion=3)

    shifted_model = mam.model()  # at (0, 0, 0), knows about shifts
    shifted_map_manager = mam.map_manager() # also at (0, 0, 0) knows shifts
    shifted_ncs_object = mam.ncs_object() # also at (0, 0, 0) and knows shifts

    Optional after boxing:  apply soft mask to map (requires soft_mask_radius)

    The maps allowed are:
      map_dict has four special keys with interpretations:
        map_manager:  full map
        map_manager_1, map_manager_2: half-maps 1 and 2
        map_manager_mask:  a mask as a map_mnager
      All other keys are any strings and are assumed to correspond to other maps

    Note:  It is permissible to call with no map_manger, but supplying
      both map_manager_1 and map_manager_2.  In this case, the working
      map_manager will be the average of map_manager_1 and map_manager_2

    Note:  mam.map_manager() contains mam.ncs_object(), so it is not necessary
    to keep both.

    Note: set wrapping of all maps to match map_manager if they differ. Set
    all to be wrapping if it is set

  '''

  def __init__(self,
               model            = None,
               map_manager      = None,  # replaces map_data
               map_manager_1    = None,  # replaces map_data_1
               map_manager_2    = None,  # replaces map_data_2
               extra_map_manager_list = None,  # replaces map_data_list
               extra_map_manager_id_list = None,  # string id's for map_managers
               ncs_object       = None,   # Overwrite ncs_objects
               wrapping         = None):  # Overwrite wrapping for all maps
    self._map_dict={}
    self._model = model
    self._shift_cart = None
    self._ncs_object = ncs_object
    self._original_origin_grid_units = None
    self._original_origin_cart = None
    self._gridding_first = None
    self._gridding_last = None
    self._solvent_content = None
    self._force_wrapping = wrapping
    self._warning_message = None

    # If map_manager_1 and map_manager_2 are supplied but no map_manager,
    #   create map_manager as average of map_manager_1 and map_manager_2

    if (map_manager_1 and map_manager_2) and (not map_manager):
      map_manager = map_manager_1.customized_copy(map_data =
        0.5 * (map_manager_1.map_data() + map_manager_2.map_data()))

    # If no map_manager now, do not do anything and make sure there
    #    was nothing else supplied

    if not map_manager:
      assert not map_manager_1 and not map_manager_2 and \
        not extra_map_manager_list
      assert not ncs_object and not model
      return  # do not do anything

    # Overwrite wrapping if requested
    # Take wrapping from map_manager otherwise for all maps

    if self._force_wrapping is None:
      wrapping = map_manager.wrapping()
    else:
      wrapping = self._force_wrapping

    if not extra_map_manager_list:
        extra_map_manager_list=[]
    for m in [map_manager, map_manager_1, map_manager_2]+ \
         extra_map_manager_list:
        if m:
          m.set_wrapping(wrapping)

    # CHECKS

    # Make sure that map_manager is either already shifted to (0, 0, 0) or has
    #   origin_shift_grid_unit of (0, 0, 0).
    assert map_manager.origin_is_zero() or \
      map_manager.origin_shift_grid_units == (0, 0, 0)

    # Normally map_manager unit_cell_crystal_symmetry should match
    #  model original_crystal_symmetry (and also usually model.crystal_symmetry)

    # Make sure we have what is expected: optional model, mm,
    # map_manager_1 and map_manager_2 or neither,
    #   optional list of extra_map_manager_list

    if extra_map_manager_list:
      if extra_map_manager_id_list:
        assert len(extra_map_manager_list) == len(extra_map_manager_id_list)
      else:
        extra_map_manager_id_list=[]
        for i in range(1,len(extra_map_manager_list)+1):
          extra_map_manager_id_list.append("extra_map_manager_%s" %(i))

    else:
      extra_map_manager_list = []
      extra_map_manager_id_list = []

    if(not [map_manager_1, map_manager_2].count(None) in [0, 2]):
      raise Sorry("None or two half-maps are required.")
    if(not map_manager):
      raise Sorry("A map is required.")

    # Make sure all map_managers have same gridding and symmetry
    for m in [map_manager_1, map_manager_2]+ \
         extra_map_manager_list:
      if m:
        if not map_manager.is_similar(m):
          raise AssertionError(map_manager.warning_message())

    # READY

    # Make a match_map_model_ncs and check unit_cell and
    #   working crystal symmetry
    #  and shift_cart for model, map, and ncs_object (if present)

    mmmn = match_map_model_ncs()
    mmmn.add_map_manager(map_manager)
    if self._model:
      mmmn.add_model(self._model, set_model_log_to_null = False) # keep the log
    if self._ncs_object:
      mmmn.add_ncs_object(self._ncs_object)

    # All ok here if it did not stop

    # Shift origin of model and map_manager and ncs_object to (0, 0, 0) with
    #    mmmn which knows about all of them

    mmmn.shift_origin(log = null_out())

    # map_manager, model, ncs_object know about shift
    map_manager = mmmn.map_manager()
    self._model = mmmn.model()  # this model knows about shift
    self._ncs_object = mmmn.ncs_object()  # ncs object also knows about shift
    self._crystal_symmetry = map_manager.crystal_symmetry()
    self._shift_cart=map_manager.shift_cart()

    if self._model:
      # Make sure model shift manager agrees with map_manager shift
      assert approx_equal(self._model.shift_cart(), map_manager.shift_cart())

    if self._ncs_object:
      # Make sure ncs_object shift manager agrees with map_manager shift
      assert approx_equal(self._ncs_object.shift_cart(),
         map_manager.shift_cart())

    # Shift origins of all other maps
    for m in [map_manager_1, map_manager_2]+\
         extra_map_manager_list:
      if m:
        m.shift_origin()

    # Transfer ncs_object to all map_managers if one is present
    if self._ncs_object:
      for m in [map_manager, map_manager_1, map_manager_2]+\
           extra_map_manager_list:
        if m:
          m.set_ncs_object(self._ncs_object)

    # Make sure all really match:
    for m in [map_manager_1, map_manager_2]+\
        extra_map_manager_list:
      if m:
        if not map_manager.is_similar(m):
          raise AssertionError(map_manager.warning_message())

    # Save origin after origin shift but before any boxing
    #    so they can be accessed easily later

    self._original_origin_grid_units = map_manager.origin_shift_grid_units
    self._original_origin_cart = tuple(
       [-x for x in map_manager.shift_cart()])

    #  Save gridding of this original map (after shifting, whole thing):
    self._gridding_first = (0, 0, 0)
    self._gridding_last = map_manager.map_data().all()

    # Holder for solvent content used in boxing and transferred to box_object
    self._solvent_content = None

    # Set up maps, model, ncs as dictionaries (same as used in r_model)

    self.set_up_map_dict(
      map_manager = map_manager,
      map_manager_1 = map_manager_1,
      map_manager_2 = map_manager_2,
      extra_map_manager_list = extra_map_manager_list,
      extra_map_manager_id_list = extra_map_manager_id_list)

  def set_up_map_dict(self,
      map_manager = None,
      map_manager_1 = None,
      map_manager_2 = None,
      extra_map_manager_list = None,
      extra_map_manager_id_list = None):

    '''
      map_dict has four special keys with interpretations:
        map_manager:  full map
        map_manager_1, map_manager_2: half-maps 1 and 2
        map_manager_mask:  a mask in a map_manager
      All other keys are any strings and are assumed to correspond to other maps
      map_manager must be present
    '''


    assert map_manager is not None
    self._map_dict={}
    self._extra_map_manager_id_list=extra_map_manager_id_list
    self._map_dict['map_manager']=map_manager
    if map_manager_1 and map_manager_2:
      self._map_dict['map_manager_1']=map_manager_1
      self._map_dict['map_manager_2']=map_manager_2
    if extra_map_manager_id_list:
      for id, m in zip(extra_map_manager_id_list,extra_map_manager_list):
        self._map_dict[id]=m

  def __repr__(self):
    text = "Map_model_manager: "
    if self.model():
      text += "\n %s" %(str(self.model()))
    info = self.get_map_manager_info()
    if self.map_manager():
      text += "\nmap_manager: %s" %(str(self.map_manager()))
    for id in info.other_map_manager_id_list:
      text += "\n%s: %s" %(id,str(self.get_map_manager(id)))
    return text


  def box_all_maps_around_model_and_shift_origin(self,
     box_cushion = 5.,
     log = sys.stdout):
    '''
       Box all maps around the model, shift origin of maps, model
       Replaces existing map_managers and shifts model in place

       Normally used immediately after setting up this map_model_manager to
       work with boxed maps and model.

    '''
    assert isinstance(self._model, model_manager)
    assert box_cushion is not None

    from cctbx.maptbx.box import around_model

    map_manager_info=self.get_map_manager_info()
    assert map_manager_info.map_manager_id is not None

    # Make box around model and apply it to model, first map
    box = around_model(
      map_manager = self._map_dict[map_manager_info.map_manager_id],
      model = self._model,
      cushion = box_cushion,
      wrapping = self._force_wrapping)

    if box.warning_message():
      print("%s" %(box.warning_message()), file = log)
    self._map_dict[map_manager_info.map_manager_id] = box.map_manager()
    self._model = box.model()
    self._shift_cart = box.map_manager().shift_cart()

    # Apply the box to all the other maps
    for id in map_manager_info.other_map_manager_id_list:
      self._map_dict[id] = box.apply_to_map(self._map_dict[id])

    # Update self._crystal_symmetry
    self._crystal_symmetry = self._model.crystal_symmetry()
    assert self._crystal_symmetry.is_similar_symmetry(
      self._map_dict[map_manager_info.map_manager_id].crystal_symmetry())

  def extra_map_manager_list(self):
     '''
       Return just the map_managers in extra_map_manager_id_list
     '''
     mm_list=[]
     for id in self.extra_map_manager_id_list():
       mm_list.append(self._map_dict[id])
     return mm_list


  def original_origin_cart(self):
    assert self._original_origin_cart is not None
    return self._original_origin_cart

  def original_origin_grid_units(self):
    assert self._original_origin_grid_units is not None
    return self._original_origin_grid_units

  def map_data(self):
    return self.map_manager().map_data()

  def map_data_1(self):
    if self.map_manager_1():
      return self.map_manager_1().map_data()

  def map_data_2(self):
    if self.map_manager_2():
      return self.map_manager_2().map_data()


  def map_data_list(self):
    map_data_list = []
    for mm in self.extra_map_manager_list():
      map_data_list.append(mm.map_data())
    return map_data_list

  def crystal_symmetry(self): return self._crystal_symmetry

  def xray_structure(self):
    if(self.model() is not None):
      return self.model().get_xray_structure()
    else:
      return None

  def ncs_object(self):
    if self.map_manager():
      return self.map_manager().ncs_object()
    else:
      return None

  def hierarchy(self): return self._model.get_hierarchy()

  def set_gridding_first(self, gridding_first):
    self._gridding_first = tuple(gridding_first)

  def set_gridding_last(self, gridding_last):
    self._gridding_last = tuple(gridding_last)

  def set_solvent_content(self, solvent_content):
    self._solvent_content = solvent_content

  def get_counts_and_histograms(self):
    self._counts = get_map_counts(
      map_data         = self.map_data(),
      crystal_symmetry = self.crystal_symmetry())
    self._map_histograms = get_map_histograms(
        data    = self.map_data(),
        n_slots = 20,
        data_1  = self.map_data_1(),
        data_2  = self.map_data_2())

  def counts(self):
    if not hasattr(self, '_counts'):
      self.get_counts_and_histograms()
    return self._counts

  def histograms(self):
    if not hasattr(self, '_map_histograms'):
      self.get_counts_and_histograms()
    return self._map_histograms

  def generate_map(self,
      output_map_file_name = None,
      map_coeffs = None,
      high_resolution = 3,
      gridding = None,
      origin_shift_grid_units = None,
      low_resolution_fourier_noise_fraction = 0,
      high_resolution_fourier_noise_fraction = 0,
      low_resolution_real_space_noise_fraction = 0,
      high_resolution_real_space_noise_fraction = 0,
      low_resolution_noise_cutoff = None,
      model = None,
      output_map_coeffs_file_name = None,
      scattering_table = 'electron',
      file_name = None,
      n_residues = 10,
      start_res = None,
      b_iso = 30,
      box_buffer = 5,
      space_group_number = 1,
      output_model_file_name = None,
      shake = None,
      random_seed = None,
      log = sys.stdout):

    '''
      Generate a map using generate_model and generate_map_coefficients

      Summary:
      --------

      Calculate a map and optionally add noise to it.  Supply map
      coefficients (miller_array object) and types of noise to add,
      along with optional gridding (nx, ny, nz), and origin_shift_grid_units.
      Optionally create map coefficients from a model and optionally
      generate a model.

      Unique aspect of this noise generation is that it can be specified
      whether the noise is local in real space (every point in a map
      gets a random value before Fourier filtering), or local in Fourier
      space (every Fourier coefficient gets a complex random offset).
      Also the relative contribution of each type of noise vs resolution
      can be controlled.

      Parameters:
      -----------

      Used in generate_map:
      -----------------------

      output_map_file_name (string, None):  Output map file (MRC/CCP4 format)
      map_coeffs (miller.array object, None) : map coefficients
      high_resolution (float, 3):      high_resolution limit (A)
      gridding (tuple (nx, ny, nz), None):  Gridding of map (optional)
      origin_shift_grid_units (tuple (ix, iy, iz), None):  Move location of
          origin of resulting map to (ix, iy, iz) before writing out
      low_resolution_fourier_noise_fraction (float, 0): Low-res Fourier noise
      high_resolution_fourier_noise_fraction (float, 0): High-res Fourier noise
      low_resolution_real_space_noise_fraction(float, 0): Low-res
          real-space noise
      high_resolution_real_space_noise_fraction (float, 0): High-res
          real-space noise
      low_resolution_noise_cutoff (float, None):  Low resolution where noise
          starts to be added


      Pass-through to generate_map_coefficients (if map_coeffs is None):
      -----------------------
      model (model.manager object, None):    model to use
      output_map_coeffs_file_name (string, None): output model file name
      high_resolution (float, 3):   High-resolution limit for map coeffs (A)
      scattering_table (choice, 'electron'): choice of scattering table
           All choices: wk1995 it1992 n_gaussian neutron electron

      Pass-through to generate_model (used if map_coeffs and model are None):
      -------------------------------

      file_name (string, None):  File containing model (PDB, CIF format)
      n_residues (int, 10):      Number of residues to include
      start_res (int, None):     Starting residue number
      b_iso (float, 30):         B-value (ADP) to use for all atoms
      box_buffer (float, 5):     Buffer (A) around model
      space_group_number (int, 1):  Space group to use
      output_model_file_name (string, None):  File for output model
      shake (float, None):       RMS variation to add (A) in shake
      random_seed (int, None):    Random seed for shake

    '''


    print("\nGenerating new map data\n", file = log)
    if self.map_manager():
      print("NOTE: replacing existing map data\n", file = log)
    if self.model() and  file_name:
      print("NOTE: using existing model to generate map data\n", file = log)
      model = self.model()
    else:
      model = None

    from iotbx.create_models_or_maps import generate_model, \
       generate_map_coefficients
    from iotbx.create_models_or_maps import generate_map as generate_map_data

    if not model and not map_coeffs:
      model = generate_model(
        file_name = file_name,
        n_residues = n_residues,
        start_res = start_res,
        b_iso = b_iso,
        box_buffer = box_buffer,
        space_group_number = space_group_number,
        output_model_file_name = output_model_file_name,
        shake = shake,
        random_seed = random_seed,
        log = log)

    if not map_coeffs:
      map_coeffs = generate_map_coefficients(model = model,
        high_resolution = high_resolution,
        output_map_coeffs_file_name = output_map_coeffs_file_name,
        scattering_table = scattering_table,
        log = log)

    mm = generate_map_data(
      output_map_file_name = output_map_file_name,
      map_coeffs = map_coeffs,
      high_resolution = high_resolution,
      gridding = gridding,
      origin_shift_grid_units = origin_shift_grid_units,
      low_resolution_fourier_noise_fraction = \
        low_resolution_fourier_noise_fraction,
      high_resolution_fourier_noise_fraction = \
        high_resolution_fourier_noise_fraction,
      low_resolution_real_space_noise_fraction = \
        low_resolution_real_space_noise_fraction,
      high_resolution_real_space_noise_fraction = \
        high_resolution_real_space_noise_fraction,
      low_resolution_noise_cutoff = low_resolution_noise_cutoff,
      log = log)

    mm.show_summary()
    self.set_up_map_dict(map_manager=mm)
    self._model = model

  def deep_copy(self):
    new_mmm = map_model_manager()

    from copy import deepcopy
    new_mmm._extra_map_manager_id_list = deepcopy(
        self._extra_map_manager_id_list)
    new_mmm._original_origin_grid_units=deepcopy(
        self._original_origin_grid_units)
    new_mmm._original_origin_cart=deepcopy(self._original_origin_cart)
    new_mmm._gridding_first=deepcopy(self._gridding_first)
    new_mmm._gridding_last=deepcopy(self._gridding_last)
    new_mmm._solvent_content=deepcopy(self._solvent_content)
    if self._model:
      new_mmm._model=self._model.deep_copy()
    else:
      new_mmm._model = None
    new_mmm._map_dict={}
    for id in self._map_dict.keys():
      new_mmm._map_dict[id]=self._map_dict[id].deep_copy()
    new_mmm._shift_cart = deepcopy(self._shift_cart)
    new_mmm._force_wrapping = deepcopy(self._force_wrapping)
    return new_mmm

  def as_r_model(self):
    return r_model( model            = self.model(),
               map_dict         = self.map_dict())

  def as_map_model_manager(self):
    '''
      Return this object (allows using .as_map_model_manager() on both
      map_model_manager objects and others including box.around_model() etc.
    '''
    return self

  def as_match_map_model_ncs(self):
    '''
      Return this object as a match_map_model_ncs
    '''
    from iotbx.map_model_manager import match_map_model_ncs
    mmmn = match_map_model_ncs()
    if self.map_manager():
      mmmn.add_map_manager(self.map_manager())
    if self.model():
      mmmn.add_model(self.model())
    if self.ncs_object():
      mmmn.add_ncs_object(self.ncs_object())
    return mmmn

  def as_box_object(self,
        original_map_data = None,
        solvent_content = None):
    '''
      Create a box_object for backwards compatibility with methods that used
       extract_box_around_model_and_map
    '''

    if solvent_content:
      self.set_solvent_content(solvent_content)

    if self.model():
       xray_structure_box = self.model().get_xray_structure()
       hierarchy = self.model().get_hierarchy()
    else:
       xray_structure_box = None
       hierarchy = None

    output_box = box_object(
      shift_cart = tuple([-x for x in self.map_manager().origin_shift_cart()]),
      xray_structure_box = xray_structure_box,
      hierarchy = hierarchy,
      ncs_object = self.ncs_object(),
      map_box = self.map_manager().map_data(),
      map_data = original_map_data,
      map_box_half_map_list = None,
      box_crystal_symmetry = self.map_manager().crystal_symmetry(),
      pdb_outside_box_msg = "",
      gridding_first = self._gridding_first,
      gridding_last = self._gridding_last,
      solvent_content = self._solvent_content,
      origin_shift_grid_units = [
         -x for x in self.map_manager().origin_shift_grid_units],
      )
    return output_box

class match_map_model_ncs:
  '''
   match_map_model_ncs

   Use: Container to hold map, model, ncs object and check
   consistency and shift origin

   Normal usage:

     Initialize empty, then read in or add a group of model.manager,
     map_manager, and ncs objects

     Read in the models, maps, ncs objects

     Shift origin to (0, 0, 0) and save position of this (0, 0, 0) point in the
        original coordinate system so that everything can be written out
        superimposed on the original locations. This is origin_shift_grid_units
        in grid units


     NOTE: modifies the model, map_manager, and ncs objects. Call with
     deep_copy() of these if originals need to be preserved.

     Input models, maps, and ncs_object must all match in crystal_symmetry,
     original (unit_cell) crystal_symmetry, and shift_cart for maps)

     If map_manager contains an ncs_object and an ncs_object is supplied,
     the map_manager receives the supplied ncs_object

  '''

  def __init__(self, ):
    self._map_manager = None
    self._model = None

  def deep_copy(self):
    new_mmmn = match_map_model_ncs()
    if self._model:
      new_mmmn.add_model(self._model.deep_copy())
    if self._map_manager:
      new_mmmn.add_map_manager(self._map_manager.deep_copy())
    return new_mmmn

  def show_summary(self, log = sys.stdout):
    print ("Summary of maps and models", file = log)
    if self._map_manager:
      print("Map summary:", file = log)
      self._map_manager.show_summary(out = log)
    if self._model:
      print("Model summary:", file = log)
      print("Residues: %s" %(
       self._model.get_hierarchy().overall_counts().n_residues), file = log)

    if self.ncs_object():
      print("NCS summary:", file = log)
      print("Operators: %s" %(
       self.ncs_object().max_operators()), file = log)

  def write_map(self, file_name = None, log = sys.stdout):
    if not self._map_manager:
      print ("No map to write out", file = log)
    elif not file_name:
      print ("Need file name to write map", file = log)
    else:
      self._map_manager.write_map(file_name = file_name)

  def write_model(self,
     file_name = None,
     log = sys.stdout):
    if not self._model:
      print ("No model to write out", file = log)
    elif not file_name:
      print ("Need file name to write model", file = log)
    else:
      # Write out model

      f = open(file_name, 'w')
      print(self._model.model_as_pdb(), file = f)
      f.close()
      print("Wrote model with %s residues to %s" %(
         self._model.get_hierarchy().overall_counts().n_residues,
         file_name), file = log)

  def crystal_symmetry(self):
    # Return crystal symmetry of map, or if not present, of model
    if self._map_manager:
      return self._map_manager.crystal_symmetry()
    elif self._model:
      return self._model.crystal_symmetry()
    else:
      return None

  def unit_cell_crystal_symmetry(self):
    # Return unit_cell crystal symmetry of map
    if self._map_manager:
      return self._map_manager.unit_cell_crystal_symmetry()
    else:
      return None

  def map_manager(self):
    return self._map_manager

  def model(self):
    return self._model

  def ncs_object(self):
    if self.map_manager():
      return self.map_manager().ncs_object()
    else:
      return None

  def add_map_manager(self, map_manager = None, log = sys.stdout):
    # Add a map and make sure its symmetry is similar to others
    self._map_manager = map_manager
    if self.model():
      self.check_model_and_set_to_match_map_if_necessary()

  def check_model_and_set_to_match_map_if_necessary(self):
    # Map, model and ncs_object all must have same symmetry and shifts at end

    if self.map_manager() and self.model():
      # Must be compatible...then set model symmetry if not set
      ok=self.map_manager().is_compatible_model(self.model(),
        require_match_unit_cell_crystal_symmetry=False)
      if ok:
        self.map_manager().set_model_symmetries_and_shift_cart_to_match_map(
          self.model())  # modifies self.model() in place
      else:
         raise AssertionError(self.map_manager().warning_message())

  def add_model(self, model = None, set_model_log_to_null = True,
     log = sys.stdout):
    # Add a model and make sure its symmetry is similar to others
    # Check that model original crystal_symmetry matches full
    #    crystal_symmetry of map
    if set_model_log_to_null:
      model.set_log(null_out())
    self._model = model
    if self.map_manager():
      self.check_model_and_set_to_match_map_if_necessary()

  def add_ncs_object(self, ncs_object = None, log = sys.stdout):
    # Add an NCS object
    # Must already have a map_manager

    assert self.map_manager() is not None
    self.map_manager().set_ncs_object(ncs_object)
    # Check to make sure its shift_cart matches
    self.check_model_and_set_to_match_map_if_necessary()

  def read_map(self, file_name = None, log = sys.stdout):
    # Read in a map and make sure its symmetry is similar to others
    mm = map_manager(file_name)
    self.add_map_manager(mm, log = log)

  def read_model(self, file_name = None, log = sys.stdout):
    print("Reading model from %s " %(file_name), file = log)
    from iotbx.pdb import input
    inp = input(file_name = file_name)
    from mmtbx.model import manager as model_manager
    model = model_manager(model_input = inp)
    self.add_model(model, log = log)


  def read_ncs_file(self, file_name = None, log = sys.stdout):
    # Read in an NCS file and make sure its symmetry is similar to others
    from mmtbx.ncs.ncs import ncs
    ncs_object = ncs()
    ncs_object.read_ncs(file_name = file_name, log = log)
    if ncs_object.max_operators()<2:
       self.ncs_object.set_unit_ncs()
    self.add_ncs_object(ncs_object)

  def set_original_origin_and_gridding(self,
      original_origin = None,
      gridding = None):
    '''
     Use map_manager to reset (redefine) the original origin and gridding
     of the map.
     You can supply just the original origin in grid units, or just the
     gridding of the full unit_cell map, or both.

     Update shift_cart for model and ncs object if present.

    '''

    assert self._map_manager is not None

    self._map_manager.set_original_origin_and_gridding(
         original_origin = original_origin,
         gridding = gridding)

    # Get the current origin shift based on this new original origin
    shift_cart = self._map_manager.shift_cart()
    if self._model:
      if self._model.shift_cart() is None:
        self._model.set_unit_cell_crystal_symmetry_and_shift_cart(
          unit_cell_crystal_symmetry = \
           self._map_manager.unit_cell_crystal_symmetry())
      self._model.set_shift_cart(shift_cart)

  def shift_origin(self, desired_origin = (0, 0, 0), log = sys.stdout):
    # shift the origin of all maps/models to desired_origin (usually (0, 0, 0))
    if not self._map_manager:
      print ("No information about origin available", file = log)
      return
    if self._map_manager.map_data().origin() == desired_origin:
      print("Origin is already at %s, no shifts will be applied" %(
       str(desired_origin)), file = log)

    # Figure out shift of model if incoming map and model already had a shift

    if self._model:

      # Figure out shift for model and make sure model and map agree
      shift_info = self._map_manager._get_shift_info(
         desired_origin = desired_origin)

      current_shift_cart = self._map_manager.grid_units_to_cart(
       tuple([-x for x in shift_info.current_origin_shift_grid_units]))
      expected_model_shift_cart = current_shift_cart

      shift_to_apply_cart = self._map_manager.grid_units_to_cart(
        shift_info.shift_to_apply)
      new_shift_cart = self._map_manager.grid_units_to_cart(
        tuple([-x for x in shift_info.new_origin_shift_grid_units]))
      new_full_shift_cart = new_shift_cart
      # shift_to_apply_cart is coordinate shift we are going to apply
      #  new_shift_cart is how to get to new location from original
      #   current_shift_cart is how to get to current location from original
      assert approx_equal(shift_to_apply_cart, [(a-b) for a, b in zip(
        new_shift_cart, current_shift_cart)])

      # Get shifts already applied to  model
      #    and check that they match map

      if self._model:
        existing_shift_cart = self._model.shift_cart()
        if existing_shift_cart is not None:
          assert approx_equal(existing_shift_cart, expected_model_shift_cart)

      if self._map_manager.origin_is_zero() and \
         expected_model_shift_cart == (0, 0, 0):
        pass # Need to set model shift_cart below

    # Apply shift to model, map and ncs object

    # Shift origin of map_manager
    self._map_manager.shift_origin(desired_origin = desired_origin)

    # Shift origin of model  Note this sets model shift_cart
    if self._model:
      self._model = self.shift_model_to_match_working_map(
        coordinate_shift = shift_to_apply_cart,
        new_shift_cart = new_full_shift_cart,
        model = self._model, log = log)

  def shift_ncs_to_match_working_map(self, ncs_object = None, reverse = False,
    coordinate_shift = None,
    new_shift_cart = None,
    log = sys.stdout):
    # Shift an ncs object to match the working map (based
    #    on self._map_manager.origin_shift_grid_units)
    if coordinate_shift is None:
      coordinate_shift = self.get_coordinate_shift(reverse = reverse)

    ncs_object = ncs_object.coordinate_offset(coordinate_shift)
    return ncs_object

  def shift_ncs_to_match_original_map(self, ncs_object = None, log = sys.stdout):
    return self.shift_ncs_to_match_working_map(ncs_object = ncs_object,
      reverse = True, log = log)

  def get_coordinate_shift(self, reverse = False):
    if reverse: # Get origin shift in grid units  ==  position of original origin
                #  on the current grid
      origin_shift = self._map_manager.origin_shift_grid_units
    else:  # go backwards
      a = self._map_manager.origin_shift_grid_units
      origin_shift = [-a[0], -a[1], -a[2]]

    coordinate_shift = []
    for shift_grid_units, spacing in zip(
       origin_shift, self._map_manager.pixel_sizes()):
      coordinate_shift.append(shift_grid_units*spacing)
    return coordinate_shift

  def shift_model_to_match_working_map(self, model = None, reverse = False,
     coordinate_shift = None,
     new_shift_cart = None,
     log = sys.stdout):

    '''
    Shift a model based on the coordinate shift for the working map.

    Optionally specify the shift to apply (coordinate shift) and the
    new value of the shift recorded in the model (new_shift_cart)
    '''

    if coordinate_shift is None:
      coordinate_shift = self.get_coordinate_shift(
       reverse = reverse)
    if new_shift_cart is None:
      new_shift_cart = coordinate_shift

    model.shift_model_and_set_crystal_symmetry(shift_cart = coordinate_shift,
      crystal_symmetry = model.crystal_symmetry())  # keep crystal_symmetry

    # Allow specifying the final shift_cart:
    if tuple(new_shift_cart) !=  tuple(coordinate_shift):
      model.set_shift_cart(new_shift_cart)

    return model

  def shift_model_to_match_original_map(self, model = None, log = sys.stdout):
    # Shift a model object to match the original map (based
    #    on -self._map_manager.origin_shift_grid_units)
    return self.shift_model_to_match_working_map(model = model, reverse = True,
      log = log)

  def as_map_model_manager(self):

    '''
      Return map_model_manager object with contents of this class
      (not a deepcopy)

    '''
    from iotbx.map_model_manager import map_model_manager
    mam = map_model_manager(
        map_manager = self.map_manager(),
        model = self.model(),
        )
    # Keep track of the gridding and solvent_content (if used) in this boxing.
    return mam

#   Misc methods XXX to be removed

def get_map_histograms(data, n_slots = 20, data_1 = None, data_2 = None):
  h0, h1, h2 = None, None, None
  data_min = None
  hmhcc = None
  if(data_1 is None):
    h0 = flex.histogram(data = data.as_1d(), n_slots = n_slots)
  else:
    data_min = min(flex.min(data_1), flex.min(data_2))
    data_max = max(flex.max(data_1), flex.max(data_2))
    h0 = flex.histogram(data = data.as_1d(), n_slots = n_slots)
    h1 = flex.histogram(data = data_1.as_1d(), data_min = data_min,
      data_max = data_max, n_slots = n_slots)
    h2 = flex.histogram(data = data_2.as_1d(), data_min = data_min,
      data_max = data_max, n_slots = n_slots)
    hmhcc = flex.linear_correlation(
      x = h1.slots().as_double(),
      y = h2.slots().as_double()).coefficient()
  return group_args(h_map = h0, h_half_map_1 = h1, h_half_map_2 = h2,
    _data_min = data_min, half_map_histogram_cc = hmhcc)

def get_map_counts(map_data, crystal_symmetry = None):
  a = map_data.accessor()
  map_counts = group_args(
    origin       = a.origin(),
    last         = a.last(),
    focus        = a.focus(),
    all          = a.all(),
    min_max_mean = map_data.as_1d().min_max_mean().as_tuple(),
    d_min_corner = maptbx.d_min_corner(map_data = map_data,
      unit_cell = crystal_symmetry.unit_cell()))
  return map_counts
