#include <sys/time.h>
#include <simtbx/diffBragg/src/diffBragg.h>
#include <assert.h>
#include <stdbool.h>

#pragma omp declare reduction(vec_double_plus : std::vector<double> : \
                              std::transform(omp_out.begin(), omp_out.end(), omp_in.begin(), omp_out.begin(), std::plus<double>())) \
                    initializer(omp_priv = decltype(omp_orig)(omp_orig.size()))

namespace simtbx {
namespace nanoBragg {

// BEGIN derivative manager
derivative_manager::derivative_manager(){}

void derivative_manager::initialize(int Npix_total, bool curvatures)
{
    //raw_pixels = af::flex_double(af::flex_grid<>(sdim,fdim));
    raw_pixels = af::flex_double(Npix_total);
    dI=0;

    // for second derivatives
    dI2=0;
    //if (curvatures)
    //raw_pixels2 = af::flex_double(af::flex_grid<>(sdim,fdim));
    raw_pixels2 = af::flex_double(Npix_total);
}

void derivative_manager::increment_image(int idx, double value, double value2, bool curvatures){
    double* floatimage = raw_pixels.begin();
    floatimage[idx] = value;

    // increment second derivatives
    if (curvatures){
        double* floatimage2 = raw_pixels2.begin();
        floatimage2[idx] = value2;
    }
}
// END derivative manager

//BEGIN origin manager
origin_manager::origin_manager(){
    FF=0;
    FdF=0;
    dFdF=0;
    FdF2=0;
    }

// BEGIN Ncells_abc manager
Ncells_manager::Ncells_manager(){}

void Ncells_manager::increment(double dI_increment, double dI2_increment){
    dI += dI_increment;
    dI2 += dI2_increment;
};

//END Ncells_abc manager

// Begin Fcell manager
Fcell_manager::Fcell_manager(){}

void Fcell_manager::increment( double value, double value2)
{
    dI += value;
    dI2 += value2;
};
// END Fcell manager

// Begin eta manager
eta_manager::eta_manager(){}

void eta_manager::increment( double value, double value2)
{
    dI += value;
    dI2 += value2;
};
// END eta manager

// begin lambda_manager
lambda_manager::lambda_manager(){}

void lambda_manager::increment( double value, double value2)
{
    dI += value;
    dI2 += value2;
};
//END lambda manager

// begin panel_manager
panel_manager::panel_manager(){}

//END panel_manager

//BEGIN unit cell manager
ucell_manager::ucell_manager(){}

void ucell_manager::increment(double value, double value2)
{
  dI += value;
  dI2 += value2;
};

// BEGIN rotation manager begin
rot_manager::rot_manager(){}

void rot_manager::set_R(){assert (false);}

void rot_manager::increment(double value, double value2)
{
  dI += value;
  dI2 += value2;
}

rotX_manager::rotX_manager(){
    value = 0;
    set_R();
}
rotZ_manager::rotZ_manager(){
    value = 0;
    set_R();
}
rotY_manager::rotY_manager(){
    value = 0;
    set_R();
}

void rotX_manager::set_R(){
    R << 1,           0,           0,
          0,  cos(value), sin(value),
          0, -sin(value), cos(value);

    dR << 0,           0,           0,
             0,  -sin(value), cos(value),
             0, -cos(value), -sin(value);

    dR2 << 0,           0,           0,
              0,  -cos(value), -sin(value),
              0,   sin(value), -cos(value);

    //SCITBX_EXAMINE(dR2[0]);
    //SCITBX_EXAMINE(dR2[1]);
    //SCITBX_EXAMINE(dR2[2]);
    //SCITBX_EXAMINE(dR2[3]);
    //SCITBX_EXAMINE(dR2[4]);
    //SCITBX_EXAMINE(dR2[5]);
    //SCITBX_EXAMINE(dR2[6]);
    //SCITBX_EXAMINE(dR2[7]);
    //SCITBX_EXAMINE(dR2[8]);

}
void rotY_manager::set_R(){
    R << cos(value),0, -sin(value),
             0,         1,             0,
            sin(value), 0, cos(value);

    dR << -sin(value),0, -cos(value),
                0,          0,             0,
                cos(value), 0, -sin(value);

    dR2 << -cos(value),0, sin(value),
              0,          0,          0,
             -sin(value), 0, -cos(value);
}
void rotZ_manager::set_R(){
    R  << cos(value),  sin(value), 0,
              -sin(value), cos(value), 0,
                         0,           0, 1;

    dR << -sin(value),  cos(value), 0,
               -cos(value), -sin(value), 0,
                           0,           0, 0;

    dR2 << -cos(value), -sin(value), 0,
               sin(value), -cos(value), 0,
                        0,           0, 0;
}
// END rot manager

// BEGIN diffBragg
diffBragg::diffBragg(const dxtbx::model::Detector& detector, const dxtbx::model::Beam& beam,
            int verbose):
    nanoBragg(detector, beam, verbose, 0)
    { // diffBragg init
    int Npanels = detector.size();
    Npix_total = Npanels * detector[0].get_image_size()[0]* detector[0].get_image_size()[1];
    fdet_vectors.clear();
    sdet_vectors.clear();
    odet_vectors.clear();
    pix0_vectors.clear();

    dF_vecs.clear();
    dS_vecs.clear();
    for (int ii=0; ii < Npanels*3; ii++){
        fdet_vectors.push_back(0);
        sdet_vectors.push_back(0);
        odet_vectors.push_back(0);
        pix0_vectors.push_back(0);

        Eigen::Vector3d vec(0,0,0);
        dF_vecs.push_back(vec);
        dS_vecs.push_back(vec);
    }

    EYE <<  1,0,0,
            0,1,0,
            0,0,1;
    eig_O << 1,0,0,
               0,1,0,
               0,0,1;
    psi = 0;

    RotMats.push_back(EYE);
    RotMats.push_back(EYE);
    RotMats.push_back(EYE);

    dRotMats.push_back(EYE);
    dRotMats.push_back(EYE);
    dRotMats.push_back(EYE);
    d2RotMats.push_back(EYE);
    d2RotMats.push_back(EYE);
    d2RotMats.push_back(EYE);

    R3.push_back(EYE);
    R3.push_back(EYE);
    R3.push_back(EYE);
    R3_2.push_back(EYE);
    R3_2.push_back(EYE);
    R3_2.push_back(EYE);

    boost::shared_ptr<rot_manager> rotX = boost::shared_ptr<rot_manager>(new rotX_manager());
    boost::shared_ptr<rot_manager> rotY = boost::shared_ptr<rot_manager>(new rotY_manager());
    boost::shared_ptr<rot_manager> rotZ = boost::shared_ptr<rot_manager>(new rotZ_manager());

    boost::shared_ptr<ucell_manager> uc1 = boost::shared_ptr<ucell_manager>(new ucell_manager());
    boost::shared_ptr<ucell_manager> uc2 = boost::shared_ptr<ucell_manager>(new ucell_manager());
    boost::shared_ptr<ucell_manager> uc3 = boost::shared_ptr<ucell_manager>(new ucell_manager());
    boost::shared_ptr<ucell_manager> uc4 = boost::shared_ptr<ucell_manager>(new ucell_manager());
    boost::shared_ptr<ucell_manager> uc5 = boost::shared_ptr<ucell_manager>(new ucell_manager());
    boost::shared_ptr<ucell_manager> uc6 = boost::shared_ptr<ucell_manager>(new ucell_manager());

    boost::shared_ptr<Ncells_manager> nc1 = boost::shared_ptr<Ncells_manager>(new Ncells_manager());
    boost::shared_ptr<Ncells_manager> nc2 = boost::shared_ptr<Ncells_manager>(new Ncells_manager());
    boost::shared_ptr<Ncells_manager> nc3 = boost::shared_ptr<Ncells_manager>(new Ncells_manager());

    boost::shared_ptr<lambda_manager> lam1 = boost::shared_ptr<lambda_manager>(new lambda_manager());
    boost::shared_ptr<lambda_manager> lam2 = boost::shared_ptr<lambda_manager>(new lambda_manager());

    boost::shared_ptr<panel_manager> orig0 = boost::shared_ptr<panel_manager>(new panel_manager());
    boost::shared_ptr<panel_manager> origX = boost::shared_ptr<panel_manager>(new panel_manager());
    boost::shared_ptr<panel_manager> origY = boost::shared_ptr<panel_manager>(new panel_manager());
    //boost::shared_ptr<origin_manager> orig0 = boost::shared_ptr<origin_manager>(new origin_manager());

    //boost::shared_ptr<Fcell_manager> fcell_man = boost::shared_ptr<Fcell_manager>(new Fcell_manager());
    fcell_man = boost::shared_ptr<Fcell_manager>(new Fcell_manager());
    fcell_man->refine_me = false;

    eta_man = boost::shared_ptr<eta_manager>(new eta_manager());
    eta_man->refine_me = false;
    mosaic_umats_prime = NULL;

    panel_rot_man = boost::shared_ptr<panel_manager>(new panel_manager());
    panel_rot_man->refine_me = false;
    panel_rot_man->F_cross_dS << 0,0,0;
    panel_rot_man->dF_cross_S << 0,0,0;
    panel_rot_manF = boost::shared_ptr<panel_manager>(new panel_manager());
    panel_rot_manF->refine_me = false;
    panel_rot_manF->F_cross_dS <<0,0,0;
    panel_rot_manF->dF_cross_S <<0,0,0;

    panel_rot_manS = boost::shared_ptr<panel_manager>(new panel_manager());
    panel_rot_manS->refine_me = false;
    panel_rot_manS->F_cross_dS <<0,0,0;
    panel_rot_manS->dF_cross_S <<0,0,0;

    panels.push_back(panel_rot_man);
    panels.push_back(orig0);
    panels.push_back(origX);
    panels.push_back(origY);
    panels.push_back(panel_rot_manF);
    panels.push_back(panel_rot_manS);

    rotX->refine_me = false;
    rotY->refine_me = false;
    rotZ->refine_me = false;
    uc1->refine_me = false;
    uc2->refine_me = false;
    uc3->refine_me = false;
    uc4->refine_me = false;
    uc5->refine_me = false;
    uc6->refine_me = false;

    nc1->refine_me = false;
    nc2->refine_me = false;
    nc3->refine_me = false;

    orig0->refine_me = false;
    orig0->dk << 0,0,1;

    origX->refine_me = false;
    origX->dk <<1,0,0;

    origY->refine_me = false;
    origY->dk <<0,1,0;

    lam1->refine_me= false;
    lam2->refine_me= false;

    lambda_managers.push_back(lam1);
    lambda_managers.push_back(lam2);

    rot_managers.push_back(rotX);
    rot_managers.push_back(rotY);
    rot_managers.push_back(rotZ);

    ucell_managers.push_back(uc1);
    ucell_managers.push_back(uc2);
    ucell_managers.push_back(uc3);
    ucell_managers.push_back(uc4);
    ucell_managers.push_back(uc5);
    ucell_managers.push_back(uc6);

    Ncells_managers.push_back(nc1);
    Ncells_managers.push_back(nc2);
    Ncells_managers.push_back(nc3);
    NABC << 1,0,0,
            0,1,0,
            0,0,1;

    O_reference <<0,0,0;

    update_oversample_during_refinement = true;
    oversample_omega = true;
    only_save_omega_kahn = false;
    compute_curvatures = true;
    isotropic_ncells = true;

    lambda_managers[0]->value = 0;
    lambda_managers[1]->value = 1;
    use_lambda_coefficients = false;
    //source_lambda0 = 0;
    //source_lambda1 = 1;

    // set ucell gradients, Bmatrix is upper triangular in diffBragg?
    for (int i=0; i <6; i++){
        ucell_managers[i]->dB << 0,0,0,
                                 0,0,0,
                                 0,0,0;
        ucell_managers[i]->dB2 << 0,0,0,
                                  0,0,0,
                                  0,0,0;
    }

    max_I_hkl << 0,0,0;
    init_raw_pixels_roi();
    initialize_managers();

    Fhkl2 = NULL;
    F_cell2 = 0;
    complex_miller = false;
    pythony_amplitudes2.clear();

    for(int pid=0; pid < Npanels; pid++){
        update_dxtbx_geoms(detector, beam, pid, 0,0,0,0,0,0,false);
    }
    linearize_Fhkl();
}

af::flex_double diffBragg::get_panel_increment( double Iincrement, double omega_pixel,
    const Eigen::Ref<const Eigen::Matrix3d>& M,
    double pix2, const Eigen::Ref<const Eigen::Vector3d>& o, const Eigen::Ref<const Eigen::Vector3d>& k_diffracted,
    double per_k, double per_k3, double per_k5, const Eigen::Ref<const Eigen::Vector3d>& V,
    const Eigen::Ref<const Eigen::Vector3d>& _dk)
{
    double G = _dk.dot(k_diffracted);
    Eigen::Vector3d dk_hat = -per_k3*G*k_diffracted + per_k*_dk;
    double coef = (M*dk_hat).dot(V);
    double coef2 = -3*pix2*per_k5*G * (o.dot(k_diffracted));
    coef2 += pix2*per_k3*(o.dot(_dk));
    double value = coef*Iincrement + coef2*Iincrement/omega_pixel;
    af::flex_double values = af::flex_double(2,0);
    values[0] = value;
    values[1] = 0;
    return values;
};



/* BEGIN panel Rot XYZ */
void diffBragg::rotate_fs_ss_vecs_3D(double panel_rot_angO, double panel_rot_angF, double panel_rot_angS){

    Eigen::Vector3d fs_vec(fdet_vector[1], fdet_vector[2], fdet_vector[3]);
    Eigen::Vector3d ss_vec(sdet_vector[1], sdet_vector[2], sdet_vector[3]);
    Eigen::Vector3d origin_vec(pix0_vector[1], pix0_vector[2], pix0_vector[3]);
    Eigen::Vector3d origin_diff_vec = origin_vec - O_reference; // difference between origin and a reference vector, this is the vector we will rotate

    Eigen::Matrix3d RO, RF, RS;
    //R0<< 0,0,0,0,0,0,0,0,0;
    //RO[1] = -odet_vector[3];
    //RO[2] = odet_vector[2];
    //RO[3] = odet_vector[3];
    //RO[5] = -odet_vector[1];
    //RO[6] = -odet_vector[2];
    //RO[7] = odet_vector[1];
    double xx,yy,zz;
    xx = odet_vector[1];
    yy = odet_vector[2];
    zz = odet_vector[3];
    RO<< 0,-zz,  yy,
        zz,  0, -xx,
       -yy, xx,   0;
    Eigen::Matrix3d RO2 = RO*RO;

    xx = fdet_vector[1];
    yy = fdet_vector[2];
    zz = fdet_vector[3];
    RF<< 0,-zz,  yy,
        zz,  0, -xx,
       -yy, xx,   0;

    //RF << 0,0,0,0,0,0,0,0,0;
    //RF[1] = -fdet_vector[3];
    //RF[2] = fdet_vector[2];
    //RF[3] = fdet_vector[3];
    //RF[5] = -fdet_vector[1];
    //RF[6] = -fdet_vector[2];
    //RF[7] = fdet_vector[1];
    Eigen::Matrix3d RF2 = RF*RF;

    xx = sdet_vector[1];
    yy = sdet_vector[2];
    zz = sdet_vector[3];
    RS<< 0,-zz,  yy,
        zz,  0, -xx,
       -yy, xx,   0;
    //RS << 0,0,0,0,0,0,0,0,0;
    //RS[1] = -sdet_vector[3];
    //RS[2] = sdet_vector[2];
    //RS[3] = sdet_vector[3];
    //RS[5] = -sdet_vector[1];
    //RS[6] = -sdet_vector[2];
    //RS[7] = sdet_vector[1];
    Eigen::Matrix3d RS2 = RS*RS;

    Eigen::Matrix3d rotO = EYE + RO*sin(panel_rot_angO) + RO2*(1-cos(panel_rot_angO));
    Eigen::Matrix3d rotF = EYE + RF*sin(panel_rot_angF) + RF2*(1-cos(panel_rot_angF));
    Eigen::Matrix3d rotS = EYE + RS*sin(panel_rot_angS) + RS2*(1-cos(panel_rot_angS));

    boost::shared_ptr<panel_manager> pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[0]);
    pan_rot->dR = RO*cos(panel_rot_angO) + RO2*sin(panel_rot_angO);
    pan_rot->value = panel_rot_angO;
    Eigen::Matrix3d ROT = (pan_rot->dR)*rotF*rotS;
    pan_rot->dF = ROT*fs_vec;
    pan_rot->dS = ROT*ss_vec;

    pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[4]);
    pan_rot->dR = RF*cos(panel_rot_angF) + RF2*sin(panel_rot_angF);
    pan_rot->value = panel_rot_angF;
    ROT = rotO*(pan_rot->dR)*rotS;
    pan_rot->dF = ROT*fs_vec;
    pan_rot->dS = ROT*ss_vec;

    pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[5]);
    pan_rot->dR = RS*cos(panel_rot_angS) + RS2*sin(panel_rot_angS);
    pan_rot->value = panel_rot_angS;
    ROT = rotO*rotF*(pan_rot->dR);
    pan_rot->dF = ROT*fs_vec;
    pan_rot->dS = ROT*ss_vec;

    ROT = rotO*rotF*rotS;
    fs_vec = ROT*fs_vec;
    ss_vec = ROT*ss_vec;
    origin_vec = O_reference + ROT*origin_diff_vec; // add the rotated difference to the reference to recover the origin of the rotated panel
    for (int i=0; i < 3; i++){
        fdet_vector[i+1] = fs_vec[i];
        sdet_vector[i+1] = ss_vec[i];
        pix0_vector[i+1] = origin_vec[i];
    }

    //int pan_mans[3] = {0,4,5};
    //for (int i_rot=0; i_rot <3; i_rot++ ){
    //    int panels_id = pan_mans[i_rot];
    //    pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[panels_id]);
    //    pan_rot->F_cross_dS = fs_vec.cross(pan_rot->dS);
    //    pan_rot->dF_cross_S = (pan_rot->dF).cross(ss_vec);
    //}


}
/* end panel Rot XYZ */


void diffBragg::update_dxtbx_geoms(
    const dxtbx::model::Detector& detector,
    const dxtbx::model::Beam& beam,
    int panel_id,
    double panel_rot_angO,
    double panel_rot_angF,
    double panel_rot_angS, double panel_offsetX, double panel_offsetY, double panel_offsetZ,
    bool force){

    /* BEAM properties first */
    detector_panel_id = panel_id;

    double temp;
    vec3 xyz;
    /* direction in 3-space of beam vector */
    xyz = beam.get_unit_s0();
    beam_vector[1] = xyz[0];
    beam_vector[2] = xyz[1];
    beam_vector[3] = xyz[2];
    unitize(beam_vector,beam_vector);

    /* central wavelength, in Angstrom */
    lambda0 = beam.get_wavelength()*1e-10;

    /* divergence, what are the DXTBX units? */
    temp = beam.get_divergence();
    if(temp>0.0) hdivrange = vdivrange = temp;

    /* assume this is photons/s, unless it is zero */
    temp = beam.get_flux();
    if(temp>0.0) flux = temp;

    /* assume this is Kahn polarization parameter */
    temp = beam.get_polarization_fraction();
    if(temp>=-1.0 && temp<=1.0) polarization = temp;

    /* dxtbx polarization points down B vector, we want the E vector */
    xyz = beam.get_polarization_normal();
    vert_vector[1] = xyz[0];
    vert_vector[2] = xyz[1];
    vert_vector[3] = xyz[2];
    unitize(vert_vector,vert_vector);
    cross_product(beam_vector,vert_vector,polar_vector);
    unitize(polar_vector,polar_vector);

    /* DETECTOR properties */
    /* size of the pixels in meters, this should not vary after instantiation */
    SCITBX_ASSERT(pixel_size == detector[panel_id].get_pixel_size()[0]/1000.);

    /* pixel count in short and fast-axis directions, should not change after instantiation */
    SCITBX_ASSERT(spixels == detector[panel_id].get_image_size()[1]);
    SCITBX_ASSERT(fpixels == detector[panel_id].get_image_size()[0]);

    /* direction in 3-space of detector axes */
    SCITBX_ASSERT (beam_convention == CUSTOM);

    fdet_vector[1] = detector[panel_id].get_fast_axis()[0];
    fdet_vector[2] = detector[panel_id].get_fast_axis()[1];
    fdet_vector[3] = detector[panel_id].get_fast_axis()[2];
    unitize(fdet_vector, fdet_vector);

    sdet_vector[1] = detector[panel_id].get_slow_axis()[0];
    sdet_vector[2] = detector[panel_id].get_slow_axis()[1];
    sdet_vector[3] = detector[panel_id].get_slow_axis()[2];
    unitize(sdet_vector,sdet_vector);
    /* set orthogonal vector to the detector pixel array */
    cross_product(fdet_vector,sdet_vector,odet_vector);
    unitize(odet_vector,odet_vector);
    if (! detector_is_righthanded)
        vector_scale(odet_vector, odet_vector, -1);

    /* dxtbx origin is location of outer corner of the first pixel */
    pix0_vector[1] = detector[panel_id].get_origin()[0]/1000.0;
    pix0_vector[2] = detector[panel_id].get_origin()[1]/1000.0;
    pix0_vector[3] = detector[panel_id].get_origin()[2]/1000.0;

    //if (panel_rot_ang != 0)
    //rotate_fs_ss_vecs(panel_rot_ang);
    int n_rot_refine = 0;
    if (panels[0]->refine_me)
        n_rot_refine ++;
    if (panels[4]->refine_me)
        n_rot_refine ++;
    if (panels[5]->refine_me)
        n_rot_refine ++;
    if (n_rot_refine  > 0 || force){
        rotate_fs_ss_vecs_3D(panel_rot_angO, panel_rot_angF, panel_rot_angS);

        /* reset orthogonal vector to the detector pixel array after rotation */
        cross_product(fdet_vector,sdet_vector,odet_vector);
        unitize(odet_vector,odet_vector);
        if (! detector_is_righthanded)
            vector_scale(odet_vector, odet_vector, -1);

        boost::shared_ptr<panel_manager> pan_rot;
        Eigen::Vector3d fs_vec(fdet_vector[1], fdet_vector[2], fdet_vector[3]);
        Eigen::Vector3d ss_vec(sdet_vector[1], sdet_vector[2], sdet_vector[3]);
        int pan_mans[3] = {0,4,5};
        for (int i_rot=0; i_rot <3; i_rot++ ){
            int panels_id = pan_mans[i_rot];
            pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[panels_id]);
            if (!detector_is_righthanded){
                pan_rot->F_cross_dS = (pan_rot->dS).cross(fs_vec);
                pan_rot->dF_cross_S = ss_vec.cross(pan_rot->dF);
            }
            else{
                pan_rot->F_cross_dS = fs_vec.cross(pan_rot->dS);
                pan_rot->dF_cross_S = (pan_rot->dF).cross(ss_vec);
            }
        }
    }

    pix0_vector[1] += panel_offsetX;///1000;
    pix0_vector[2] += panel_offsetY;///1000;
    pix0_vector[3] += panel_offsetZ;///1000;

    Fclose = Xclose = -dot_product(pix0_vector, fdet_vector);
    Sclose = Yclose = -dot_product(pix0_vector, sdet_vector);
    close_distance = distance = dot_product(pix0_vector, odet_vector);

    /* set beam centre */
    Eigen::Matrix3d dmat;
    dmat<<fdet_vector[1], sdet_vector[1], pix0_vector[1]*1000,
          fdet_vector[2], sdet_vector[2], pix0_vector[2]*1000,
          fdet_vector[3], sdet_vector[3], pix0_vector[3]*1000;
    Eigen::Matrix3d Dmat = dmat.inverse();
    Eigen::Vector3d s0(beam.get_s0()[0], beam.get_s0()[1], beam.get_s0()[2]);
    Eigen::Vector3d dxtbx_v = Dmat*s0;
    SCITBX_ASSERT(dxtbx_v[2] > 0);

    double rotated_center_x = dxtbx_v[0] / dxtbx_v[2];
    double rotated_center_y = dxtbx_v[1] / dxtbx_v[2];

    scitbx::vec2<double> dials_bc = detector[panel_id].get_beam_centre(beam.get_s0());
    dials_bc[0] = rotated_center_x;
    dials_bc[1] = rotated_center_y;
    Xbeam = dials_bc[0]/1000.0;
    Ybeam = dials_bc[1]/1000.0;

    /* detector sensor layer properties */
    detector_thick = detector[panel_id].get_thickness();
    temp = detector[panel_id].get_mu();        // is this really a mu? or mu/rho ?
    if(temp>0.0) detector_attnlen = 1.0/temp;

    /* quantum_gain = amp_gain * electrooptical_gain, does not include capture_fraction */
    quantum_gain = detector[panel_id].get_gain();
    //adc_offset = detector[panel_id].ADC_OFFSET;

    /* SPINDLE properties */

    /* By default align the rotation axis with the detector fast direction */
    spindle_vector[1] = fdet_vector[1];
    spindle_vector[2] = fdet_vector[2];
    spindle_vector[3] = fdet_vector[3];
    unitize(spindle_vector,spindle_vector);

    /* OMG So important otherwise center walks */
    ORGX=NAN;
    ORGY=NAN;
    
    user_beam=true;

    init_beam();
    init_beamcenter();
    update_beamcenter();

    int pan_rot_ids[3] = {0,4,5};
    boost::shared_ptr<panel_manager> pan;
    for (int ii=0; ii < 3; ii++){
        fdet_vectors[panel_id*3 + ii] = fdet_vector[ii+1];
        sdet_vectors[panel_id*3 + ii] = sdet_vector[ii+1];
        pix0_vectors[panel_id*3 + ii] = pix0_vector[ii+1];
        odet_vectors[panel_id*3 + ii] = odet_vector[ii+1];

        int i_rot = pan_rot_ids[ii];
        pan = boost::dynamic_pointer_cast<panel_manager>(panels[i_rot]);
        dF_vecs[panel_id*3 + ii] = pan->dF;
        dS_vecs[panel_id*3 + ii] = pan->dS;
    }

    //SCITBX_EXAMINE(Yclose);
    //SCITBX_EXAMINE(Xclose);
    //SCITBX_EXAMINE(Ybeam);
    //SCITBX_EXAMINE(Xbeam);
    //SCITBX_EXAMINE(distance);
    //SCITBX_EXAMINE(close_distance);
    //printf("Done updating!\n");
    SCITBX_ASSERT(close_distance > 0);
    }

void diffBragg::init_raw_pixels_roi(){
    //int fdim = roi_xmax-roi_xmin+1;
    //int sdim = roi_ymax-roi_ymin+1;
    //raw_pixels_roi = af::flex_double(af::flex_grid<>(sdim,fdim));
    raw_pixels_roi = af::flex_double(Npix_total);
}

void diffBragg::initialize_managers(){
    int fdim = roi_xmax-roi_xmin+1;
    int sdim = roi_ymax-roi_ymin+1;
    for (int i_rot=0; i_rot < 3; i_rot++){
        if (rot_managers[i_rot]->refine_me){
            rot_managers[i_rot]->initialize(Npix_total, compute_curvatures);
            update_rotmats_on_device = true;
        }
    }
    for (int i_uc=0; i_uc < 6; i_uc++){
        if (ucell_managers[i_uc]->refine_me){
            update_dB_matrices_on_device = true;
            ucell_managers[i_uc]->initialize(Npix_total, compute_curvatures);
        }
    }
    for (int i_nc=0; i_nc < 3; i_nc ++){
        if (Ncells_managers[i_nc]->refine_me)
            Ncells_managers[i_nc]->initialize(Npix_total, compute_curvatures);
    }

    boost::shared_ptr<panel_manager> pan_orig;
    for (int i_pan_orig=0; i_pan_orig  < 3; i_pan_orig ++){
        pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[1+i_pan_orig]);
        if (pan_orig->refine_me){
            pan_orig->initialize(Npix_total, compute_curvatures);
            update_detector_on_device=true;
        }
    }

    if (fcell_man->refine_me){
        fcell_man->initialize(Npix_total, compute_curvatures);
        update_Fhkl_on_device = true;
        }

    if (eta_man->refine_me){
        eta_man->initialize(Npix_total, compute_curvatures);
        update_umats_on_device=true;
        vectorize_umats();
        }

    for (int i_lam=0; i_lam < 2; i_lam++){
        if (lambda_managers[i_lam]->refine_me)
            lambda_managers[i_lam]->initialize(Npix_total, compute_curvatures);
    }

    int panrot_manager_indices[3] = {0,4,5};
    boost::shared_ptr<panel_manager> pan_rot;
    for (int i=0; i< 3; i++){
        int manager_idx = panrot_manager_indices[i];
        pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[manager_idx]);
        if (pan_rot->refine_me){
            update_panel_deriv_vecs_on_device=true;
            pan_rot->initialize(Npix_total, compute_curvatures);
        }
    }
}

void diffBragg::vectorize_umats(){
    /* vector store two copies of Umats, one unperturbed for reference */
    if (UMATS.size() > 0){
        UMATS.clear();
        UMATS_RXYZ.clear();
    }
    if (UMATS_prime.size()> 0){
        UMATS_prime.clear();
        UMATS_RXYZ_prime.clear();
    }
    for(mos_tic=0;mos_tic<mosaic_domains;++mos_tic){
        double uxx,uxy,uxz,uyx,uyy,uyz,uzx,uzy,uzz;
        uxx = mosaic_umats[mos_tic*9+0];
        uxy = mosaic_umats[mos_tic*9+1];
        uxz = mosaic_umats[mos_tic*9+2];
        uyx = mosaic_umats[mos_tic*9+3];
        uyy = mosaic_umats[mos_tic*9+4];
        uyz = mosaic_umats[mos_tic*9+5];
        uzx = mosaic_umats[mos_tic*9+6];
        uzy = mosaic_umats[mos_tic*9+7];
        uzz = mosaic_umats[mos_tic*9+8];
        Eigen::Matrix3d U;
        U << uxx, uxy, uxz,
             uyx, uyy, uyz,
             uzx, uzy, uzz;
        UMATS.push_back(U);
        UMATS_RXYZ.push_back(U);
        if (eta_man->refine_me){
            SCITBX_ASSERT(mosaic_umats_prime != NULL);
            uxx = mosaic_umats_prime[mos_tic*9+0];
            uxy = mosaic_umats_prime[mos_tic*9+1];
            uxz = mosaic_umats_prime[mos_tic*9+2];
            uyx = mosaic_umats_prime[mos_tic*9+3];
            uyy = mosaic_umats_prime[mos_tic*9+4];
            uyz = mosaic_umats_prime[mos_tic*9+5];
            uzx = mosaic_umats_prime[mos_tic*9+6];
            uzy = mosaic_umats_prime[mos_tic*9+7];
            uzz = mosaic_umats_prime[mos_tic*9+8];
            Eigen::Matrix3d Up;
            Up << uxx, uxy, uxz,
                 uyx, uyy, uyz,
                 uzx, uzy, uzz;
            UMATS_RXYZ_prime.push_back(Up);
            UMATS_prime.push_back(Up);
        }
    }
}

void diffBragg::refine(int refine_id){
    int fdim = roi_xmax-roi_xmin+1;
    int sdim = roi_ymax-roi_ymin+1;
    if (refine_id >= 0 && refine_id < 3  ){
        // 3 possitle rotation managers (rotX, rotY, rotZ)
        rot_managers[refine_id]->refine_me=true;
        rot_managers[refine_id]->initialize(Npix_total, compute_curvatures);
    }
    else if (refine_id >=3 and refine_id < 9 ){
        // 6 possible unit cell managers (a,b,c,al,be,ga)
        ucell_managers[refine_id-3]->refine_me=true;
        ucell_managers[refine_id-3]->initialize(Npix_total, compute_curvatures);
    }
    else if (refine_id==9){
        for (int i_nc=0; i_nc < 3; i_nc ++){
            Ncells_managers[i_nc]->refine_me=true;
            Ncells_managers[i_nc]->initialize(Npix_total, compute_curvatures);
        }
    }
    else if (refine_id==10){
        boost::shared_ptr<panel_manager> pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[1]);
        pan_orig->refine_me=true;
        pan_orig->initialize(Npix_total, compute_curvatures);
    }
    else if(refine_id==11){
        fcell_man->refine_me=true;
        fcell_man->initialize(Npix_total, compute_curvatures);
    }

    else if (refine_id==12 || refine_id==13){
        use_lambda_coefficients = true;
        int i_lam = refine_id-12;
        lambda_managers[i_lam]->refine_me=true;
        lambda_managers[i_lam]->initialize(Npix_total, compute_curvatures);
    }

    else if (refine_id==14){
        boost::shared_ptr<panel_manager> pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[0]);
        pan_rot->refine_me=true;
        rotate_fs_ss_vecs_3D(0,0,0);
        pan_rot->initialize(Npix_total, compute_curvatures);
    }

    else if (refine_id==15){
        boost::shared_ptr<panel_manager> pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[2]);
        pan_orig->refine_me=true;
        pan_orig->initialize(Npix_total, compute_curvatures);
    }

    else if (refine_id==16){
        boost::shared_ptr<panel_manager> pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[3]);
        pan_orig->refine_me=true;
        pan_orig->initialize(Npix_total, compute_curvatures);
    }
    else if (refine_id==17){
        boost::shared_ptr<panel_manager> pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[4]);
        pan_rot->refine_me=true;
        rotate_fs_ss_vecs_3D(0,0,0);
        pan_rot->initialize(Npix_total, compute_curvatures);
    }
    else if (refine_id==18){
        boost::shared_ptr<panel_manager> pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[5]);
        pan_rot->refine_me=true;
        rotate_fs_ss_vecs_3D(0,0,0);
        pan_rot->initialize(Npix_total, compute_curvatures);
    }
    else if (refine_id==19){
        eta_man->refine_me=true;
        SCITBX_ASSERT(mosaic_umats_prime != NULL);
        eta_man->initialize(Npix_total, compute_curvatures);
    }
}

void diffBragg::init_Fhkl2()
{
    /* This should only be called if init_Fhkl has already been called with python indices/amplitudes*/
    /* free any previous allocations */
    if(Fhkl2 != NULL) {
        for (h0=0; h0<=h_range;h0++) {
            for (k0=0; k0<=k_range;k0++) {
                free(Fhkl2[h0][k0]);
            }
            free(Fhkl2[h0]);
        }
        free(Fhkl2);
    }

    /* allocate memory for 3d arrays */
    Fhkl2 = (double***) calloc(h_range+1,sizeof(double**));
    if(Fhkl2==NULL){perror("ERROR");exit(9);};
    for (h0=0; h0<=h_range;h0++) {
        Fhkl2[h0] = (double**) calloc(k_range+1,sizeof(double*));
        if(Fhkl2[h0]==NULL){perror("ERROR");exit(9);};
        for (k0=0; k0<=k_range;k0++) {
            Fhkl2[h0][k0] = (double*) calloc(l_range+1,sizeof(double));
            if(Fhkl2[h0][k0]==NULL){perror("ERROR");exit(9);};
        }
    }
    for (h0=0; h0<h_range;h0++) {
        for (k0=0; k0<k_range;k0++) {
            for (l0=0; l0<l_range;l0++) {
                Fhkl2[h0][k0][l0] = 0;
            }
        }
    }
    Fhkl2[-h_min][-k_min][-l_min] = 0;

    if(verbose) printf("initializing Fhkl2 with pythony indices and amplitudes\n");
    miller_t hkl;
    for (i=0; i < pythony_indices.size(); ++i)
    {
        hkl = pythony_indices[i];
        F_cell = pythony_amplitudes2[i];
        h0 = hkl[0];
        k0 = hkl[1];
        l0 = hkl[2];
        Fhkl2[h0-h_min][k0-k_min][l0-l_min]=F_cell;
    }
    Fhkl2[-h_min][-k_min][-l_min] = 0;
    if(verbose) printf("done initializing Fhkl2:\n");
}


void diffBragg::set_ucell_derivative_matrix(int refine_id, af::shared<double> const& value){
    int ucell_param_idx = refine_id-3;  // its just how the API works, pass in 3 for first ucell matrix
    if (ucell_param_idx < 0 || ucell_param_idx > 5)
      printf("WARNING, passing in wrong refine_id for unit cell parameter (should be 3-8).\nNothing done.\n");
    else
        ucell_managers[ucell_param_idx]->dB <<
                        value[0], value[1], value[2],
                        value[3], value[4], value[5],
                        value[6], value[7], value[8];
}

void diffBragg::set_ucell_second_derivative_matrix(int refine_id, af::shared<double> const& value){
    int ucell_param_idx = refine_id-3;  // its just how the API works, pass in 3 for first ucell matrix
    if (ucell_param_idx < 0 || ucell_param_idx > 5)
      printf("WARNING, passing in wrong refine_id for unit cell parameter (should be 3-8).\nNothing done.\n");
    else
        ucell_managers[ucell_param_idx]->dB2 <<
                        value[0], value[1], value[2],
                        value[3], value[4], value[5],
                        value[6], value[7], value[8];
}

/* Begin parameter set/get */

// TODO : rename set_value and get_value because they dont apply to ucell derivatives...
// this function will get exeedingly complicated because it will try to ensure all the dependent parameters get
// adjusted when we update a given parameter that we are refining
// For example updating Ncells_abc should also update oversample, and should also update xtal_size
void diffBragg::set_ncells_values( boost::python::tuple const& values){
    Na = boost::python::extract<double>(values[0]);
    Nb = boost::python::extract<double>(values[1]);
    Nc = boost::python::extract<double>(values[2]);
    Ncells_managers[0]->value=Na;
    Ncells_managers[1]->value=Nb;
    Ncells_managers[2]->value=Nc;
    xtal_size_x = -1;
    xtal_size_y = -1;
    xtal_size_z = -1;
    if (update_oversample_during_refinement)
       update_oversample();
    NABC(0,0) = Na;
    NABC(1,1) = Nb;
    NABC(2,2) = Nc;
}

boost::python::tuple diffBragg::get_ncells_values(){
    boost::python::tuple values;
    values = boost::python::make_tuple( NABC(0,0),  NABC(1,1), NABC(2,2));
    return values;
}


void diffBragg::set_value( int refine_id, double value ){
    if (refine_id < 3){
        rot_managers[refine_id]->value = value;
        rot_managers[refine_id]->set_R();
    }
    if (refine_id==9){
        Ncells_managers[0]->value = value;
        Na=value;
        Nb=value;
        Nc=value;
        xtal_size_x = -1;
        xtal_size_y = -1;
        xtal_size_z = -1;
        //TODO make me optional!
        if (update_oversample_during_refinement)
            update_oversample();
        NABC(0,0) = value;
        NABC(1,1) = value;
        NABC(2,2) = value;
    }
}

double diffBragg::get_value(int refine_id){
    double value(0);
    if (refine_id < 3)
        value = rot_managers[refine_id]->value;
    else if (refine_id ==9)
        value = Ncells_managers[0]->value;
    return value;
}
/* End parameter set/get */

af::flex_double diffBragg::get_derivative_pixels(int refine_id){

    SCITBX_ASSERT(refine_id >=0 && refine_id <= 19);

    if (refine_id>=0 and refine_id < 3){
        SCITBX_ASSERT(rot_managers[refine_id]->refine_me);
        return rot_managers[refine_id]->raw_pixels;
        }
    else if(refine_id >=3 && refine_id < 9){
        int i_uc = refine_id-3;
        SCITBX_ASSERT(i_uc >= 0);
        SCITBX_ASSERT(i_uc < 6);
        SCITBX_ASSERT(ucell_managers[i_uc]->refine_me);
        return ucell_managers[i_uc]->raw_pixels;
        }
    else if (refine_id==9)
        return Ncells_managers[0]->raw_pixels;
    else if (refine_id==10){
        boost::shared_ptr<panel_manager> pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[1]);
        return pan_orig->raw_pixels;
        }
    else if (refine_id==11)
        return fcell_man->raw_pixels;
    else if (refine_id==12)
        return lambda_managers[0]->raw_pixels;
    else if  (refine_id==13)
        return lambda_managers[1]->raw_pixels;
    else if (refine_id==14){
        boost::shared_ptr<panel_manager> pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[0]);
        return pan_rot->raw_pixels;
    }
    else if (refine_id==15){
        boost::shared_ptr<panel_manager> pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[2]);
        return pan_orig->raw_pixels;
    }
    else if(refine_id==16){
        boost::shared_ptr<panel_manager> pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[3]);
        return pan_orig->raw_pixels;
    }
    else if(refine_id==17){
        boost::shared_ptr<panel_manager> pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[4]);
        return pan_rot->raw_pixels;
    }
    else if (refine_id==18){
        boost::shared_ptr<panel_manager> pan_rot = boost::dynamic_pointer_cast<panel_manager>(panels[5]);
        return pan_rot->raw_pixels;
    }
    else // refine_id == 19
        return eta_man->raw_pixels;
}


af::flex_double diffBragg::get_second_derivative_pixels(int refine_id){
    if (refine_id>=0 and refine_id < 3){
        SCITBX_ASSERT(rot_managers[refine_id]->refine_me);
        return rot_managers[refine_id]->raw_pixels2;
        }
    else if(refine_id >=3 && refine_id < 9){
        int i_uc = refine_id-3;
        SCITBX_ASSERT(i_uc >= 0);
        SCITBX_ASSERT(i_uc < 6);
        SCITBX_ASSERT(ucell_managers[i_uc]->refine_me);
        return ucell_managers[i_uc]->raw_pixels2;
        }
    else if (refine_id == 9)
        return Ncells_managers[0]->raw_pixels2;
    else if (refine_id==10){
        boost::shared_ptr<panel_manager> pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[1]);
        return pan_orig->raw_pixels2;}
    else if (refine_id==15){
        boost::shared_ptr<panel_manager> pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[2]);
        return pan_orig->raw_pixels2;}
    else if (refine_id==16){
        boost::shared_ptr<panel_manager> pan_orig = boost::dynamic_pointer_cast<panel_manager>(panels[3]);
        return pan_orig->raw_pixels2;}
    else
        return fcell_man->raw_pixels2;
}

boost::python::tuple diffBragg::get_ncells_derivative_pixels(){
    SCITBX_ASSERT(Ncells_managers[0]->refine_me);
    SCITBX_ASSERT(Ncells_managers[1]->refine_me);
    SCITBX_ASSERT(Ncells_managers[2]->refine_me);
    boost::python::tuple derivative_pixels;
    derivative_pixels = boost::python::make_tuple(Ncells_managers[0]->raw_pixels,
        Ncells_managers[1]->raw_pixels, Ncells_managers[2]->raw_pixels);
    return derivative_pixels;
}

boost::python::tuple diffBragg::get_lambda_derivative_pixels(){
    SCITBX_ASSERT(lambda_managers[0]->refine_me || lambda_managers[1]->refine_me);

    boost::python::tuple derivative_pixels;
    if (lambda_managers[0]->refine_me && lambda_managers[1]->refine_me){
        derivative_pixels = boost::python::make_tuple(lambda_managers[0]->raw_pixels,
            lambda_managers[1]->raw_pixels);
    }
    else{
        if (lambda_managers[0]->refine_me)
            derivative_pixels = boost::python::make_tuple(lambda_managers[0]->raw_pixels);
        else if (lambda_managers[1]->refine_me)
            derivative_pixels = boost::python::make_tuple(lambda_managers[1]->raw_pixels);
    }
    return derivative_pixels;
}

boost::python::tuple diffBragg::get_ncells_second_derivative_pixels(){
    SCITBX_ASSERT(Ncells_managers[0]->refine_me);
    SCITBX_ASSERT(Ncells_managers[1]->refine_me);
    SCITBX_ASSERT(Ncells_managers[2]->refine_me);
    boost::python::tuple second_derivative_pixels;
    second_derivative_pixels = boost::python::make_tuple(Ncells_managers[0]->raw_pixels2, Ncells_managers[1]->raw_pixels2, Ncells_managers[2]->raw_pixels2);
    return second_derivative_pixels;
}

void diffBragg::zero_raw_pixel_rois(){
    init_raw_pixels_roi();
    initialize_managers();
}

/* polarization factor */
/* override this to store variables needed for derivatives */
double diffBragg::polarization_factor(double kahn_factor, double *__incident, double *__diffracted, double *__axis)
{
    double cos2theta,cos2theta_sqr,sin2theta_sqr;
    //double psi=0.0;
    double E_in[4];
    double B_in[4];

    unitize(__incident,__incident);
    unitize(__diffracted,__diffracted);
    //unitize(__axis,__axis);

    /* component of diffracted unit vector along incident beam unit vector */
    cos2theta = dot_product(__incident,__diffracted);
    cos2theta_sqr = cos2theta*cos2theta;
    sin2theta_sqr = 1-cos2theta_sqr;
    //double _u = cos2theta_sqr;

    double _psi=0;
    if(kahn_factor != 0.0){
        //SCITBX_EXAMINE(kahn_factor);
        /* tricky bit here is deciding which direciton the E-vector lies in for each source
           here we assume it is closest to the "axis" defined above */

        /* cross product to get "vertical" axis that is orthogonal to the cannonical "polarization" */
        cross_product(__axis,__incident,B_in);
        /* make it a unit vector */
        unitize(B_in,B_in);

        /* cross product with incident beam to get E-vector direction */
        cross_product(__incident,B_in,E_in);
        /* make it a unit vector */
        unitize(E_in,E_in);

        /* get components of diffracted ray projected onto the E-B plane */
        double _kEi = dot_product(__diffracted,E_in);
        double _kBi = dot_product(__diffracted,B_in);

        /* compute the angle of the diffracted ray projected onto the incident E-B plane */
        _psi = -atan2(_kBi,_kEi);
    }
    /* correction for polarized incident beam */
    return 0.5*(1.0 + cos2theta_sqr - kahn_factor*cos(2*_psi)*sin2theta_sqr);
}

void diffBragg::set_mosaic_blocks_prime(af::shared<mat3> umat_in){
    /* free any previous allocations */
    if(mosaic_umats_prime != NULL) free(mosaic_umats_prime);

    /* allocate enough space */
    SCITBX_ASSERT(mosaic_domains == umat_in.size());
    mosaic_umats_prime = (double *) calloc(mosaic_domains+10,9*sizeof(double));

    /* now actually import the orientation of each domain */
    for(mos_tic=0;mos_tic<mosaic_domains;++mos_tic){
      int offset = 9 * mos_tic;
      mat3& domain = umat_in[mos_tic];
      mosaic_umats_prime[0+offset]=domain[0];mosaic_umats_prime[1+offset]=domain[1];mosaic_umats_prime[2+offset]=domain[2];
      mosaic_umats_prime[3+offset]=domain[3];mosaic_umats_prime[4+offset]=domain[4];mosaic_umats_prime[5+offset]=domain[5];
      mosaic_umats_prime[6+offset]=domain[6];mosaic_umats_prime[7+offset]=domain[7];mosaic_umats_prime[8+offset]=domain[8];
    }
    if(verbose) printf("  imported a total of %d mosaic domain derivative Umats\n",mosaic_domains);
}

void diffBragg::add_diffBragg_spots(){
    int npix = fpixels*spixels;
    af::shared<size_t> pfs(npix*3);
    for (int s=0; s <spixels; s++){
        for (int f=0; f <fpixels; f++){
            int i = s*fpixels + f;
            pfs[i] = detector_panel_id;
            pfs[i+1] = f;
            pfs[i+2] = s;
        }
    }
    add_diffBragg_spots(pfs);
    double* floatimage = raw_pixels.begin();
    double* floatimage_roi = raw_pixels_roi.begin();
    for (int i=0; i< npix;i++)
        floatimage[i] = floatimage_roi[i];
}


// BEGIN diffBragg_add_spots
void diffBragg::add_diffBragg_spots(const af::shared<size_t>& panels_fasts_slows){
    int Npix_to_model = panels_fasts_slows.size()/3;
    SCITBX_ASSERT(Npix_to_model <= Npix_total);
    double * floatimage_roi = raw_pixels_roi.begin();

    diffBragg_rot_mats();

    /* make sure we are normalizing with the right number of sub-steps */
    steps = phisteps*mosaic_domains*oversample*oversample;
    subpixel_size = pixel_size/oversample;
    const int Nsteps = oversample*oversample*detector_thicksteps*sources*phisteps*mosaic_domains;
    int* subS_pos = new int[Nsteps];
    int* subF_pos = new int[Nsteps];
    int* thick_pos = new int[Nsteps];
    int* source_pos = new int[Nsteps];
    int* phi_pos = new int[Nsteps];
    int* mos_pos = new int[Nsteps];

    diffBragg_list_steps(subS_pos, subF_pos, thick_pos, source_pos, phi_pos, mos_pos);

    int pan_rot_ids[3] = {0,4,5};
    int pan_orig_ids[3] = {1,2,3};

    int _printout_fpixel = printout_fpixel;
    int _printout_spixel = printout_spixel;
    bool _printout = printout;
    double _default_F = default_F;

    std::vector<bool> refine_pan_rot(3, false);
    std::vector<bool> refine_pan_orig(3,false);
    std::vector<bool> refine_lambda(2,false);
    std::vector<bool> refine_Bmat(6,false);
    std::vector<bool> refine_Umat(3,false);
    std::vector<bool> refine_Ncells(3,false);

    for(int i_pan=0;i_pan < 3; i_pan++){
        int i_pan_rot = pan_rot_ids[i_pan];
        int i_pan_orig = pan_orig_ids[i_pan];
        if (panels[i_pan_rot]->refine_me)
            refine_pan_rot[i_pan] = true;
        if (panels[i_pan_orig]-> refine_me)
            refine_pan_orig[i_pan] = true;
    }
    for (int i_uc = 0; i_uc < 6; i_uc++){
        if (ucell_managers[i_uc]->refine_me)
            refine_Bmat[i_uc] = true;
    }

    for (int i_rot =0; i_rot< 3; i_rot ++){
        if (rot_managers[i_rot]->refine_me)
            refine_Umat[i_rot] = true;
    }

    for (int i_lam=0; i_lam< 2; i_lam++){
        if (lambda_managers[i_lam]->refine_me)
            refine_lambda[i_lam] = true;
    }

    if (Ncells_managers[0]->refine_me){
        refine_Ncells[0] = true;
        if (! isotropic_ncells){
            refine_Ncells[1] = true;
            refine_Ncells[2] = true;
        }
    }

    Eigen::Vector3d eig_spindle_vec(spindle_vector[1], spindle_vector[2], spindle_vector[3]);
    Eigen::Vector3d _polarization_axis(polar_vector[1], polar_vector[2], polar_vector[3]);

    std::vector<Eigen::Matrix3d,Eigen::aligned_allocator<Eigen::Matrix3d> > dB_Mats;
    std::vector<Eigen::Matrix3d,Eigen::aligned_allocator<Eigen::Matrix3d> > dB2_Mats;
    for(int i_uc=0; i_uc< 6; i_uc++){
        dB_Mats.push_back(ucell_managers[i_uc]->dB);
        dB2_Mats.push_back(ucell_managers[i_uc]->dB2);
    }

    std::vector<unsigned int> panels_fasts_slows_vec(panels_fasts_slows.begin(), panels_fasts_slows.begin() + panels_fasts_slows.size()) ;//(panels_fasts_slows.size());

    image_type image(Npix_to_model,0.0);
    image_type d_Umat_images(Npix_to_model*3,0.0);
    image_type d2_Umat_images(Npix_to_model*3,0.0);
    image_type d_Bmat_images(Npix_to_model*6,0.0);
    image_type d2_Bmat_images(Npix_to_model*6,0.0);
    image_type d_Ncells_images(Npix_to_model*3,0.0);
    image_type d2_Ncells_images(Npix_to_model*3,0.0);
    image_type d_fcell_images(Npix_to_model,0.0);
    image_type d2_fcell_images(Npix_to_model,0.0);
    image_type d_eta_images(Npix_to_model,0.0);
    image_type d_lambda_images(Npix_to_model*2,0.0);
    image_type d2_lambda_images(Npix_to_model*2,0.0);
    image_type d_panel_rot_images(Npix_to_model*3,0.0);
    image_type d2_panel_rot_images(Npix_to_model*3,0.0);
    image_type d_panel_orig_images(Npix_to_model*3,0.0);
    image_type d2_panel_orig_images(Npix_to_model*3,0.0);

    struct timeval t1,t2;
    gettimeofday(&t1,0 );
    if (! use_cuda){
        diffBragg::diffBragg_sum_over_steps(
            Npix_to_model, panels_fasts_slows_vec,
            image,
            d_Umat_images, d2_Umat_images,
            d_Bmat_images, d2_Bmat_images,
            d_Ncells_images, d2_Ncells_images,
            d_fcell_images,  d2_fcell_images,
            d_eta_images,
            d_lambda_images, d2_lambda_images,
            d_panel_rot_images,  d2_panel_rot_images,
            d_panel_orig_images,  d2_panel_orig_images,
            subS_pos,  subF_pos,  thick_pos,
            source_pos,  phi_pos,  mos_pos,
            Nsteps, _printout_fpixel, _printout_spixel, _printout, _default_F,
            oversample, oversample_omega, subpixel_size, pixel_size,
            detector_thickstep, detector_thick, close_distance, detector_attnlen,
            use_lambda_coefficients, lambda_managers[0]->value, lambda_managers[1]->value,
            eig_U, eig_O, eig_B, RXYZ,
            dF_vecs,
            dS_vecs,
            UMATS_RXYZ,
            UMATS_RXYZ_prime,
            RotMats, dRotMats, d2RotMats,
            UMATS,
            dB_Mats, dB2_Mats,
            source_X,  source_Y,  source_Z,  source_lambda,  source_I,
            polarization,
            Na, Nb, Nc,
            phi0, phistep,
            eig_spindle_vec, _polarization_axis,
            h_range, k_range, l_range,
            h_max, h_min, k_max, k_min, l_max, l_min, dmin,
            fudge, complex_miller, verbose, only_save_omega_kahn,
            isotropic_ncells, compute_curvatures,
            FhklLinear, Fhkl2Linear,
            refine_Bmat, refine_Ncells, refine_pan_orig, refine_pan_rot,
            fcell_man->refine_me, refine_lambda, eta_man->refine_me, refine_Umat,
            fdet_vectors, sdet_vectors,
            odet_vectors, pix0_vectors,
            nopolar, point_pixel, fluence, r_e_sqr, spot_scale);
        }
    else { // we are using cuda
       diffBragg_loopy(
           Npix_to_model, panels_fasts_slows_vec,
           image,
           d_Umat_images, d2_Umat_images,
           d_Bmat_images, d2_Bmat_images,
           d_Ncells_images, d2_Ncells_images,
           d_fcell_images,  d2_fcell_images,
           d_eta_images,
           d_lambda_images, d2_lambda_images,
           d_panel_rot_images,  d2_panel_rot_images,
           d_panel_orig_images,  d2_panel_orig_images,
           subS_pos,  subF_pos,  thick_pos,
           source_pos,  phi_pos,  mos_pos,
           Nsteps, _printout_fpixel, _printout_spixel, _printout, _default_F,
           oversample, oversample_omega, subpixel_size, pixel_size,
           detector_thickstep, detector_thick, close_distance, detector_attnlen,
           use_lambda_coefficients, lambda_managers[0]->value, lambda_managers[1]->value,
           eig_U, eig_O, eig_B, RXYZ,
           dF_vecs,
           dS_vecs,
           UMATS_RXYZ,
           UMATS_RXYZ_prime,
           RotMats, dRotMats, d2RotMats,
           UMATS,
           dB_Mats, dB2_Mats,
           source_X,  source_Y,  source_Z,  source_lambda,  source_I,
           polarization,
           Na, Nb, Nc,
           phi0, phistep,
           eig_spindle_vec, _polarization_axis,
           h_range, k_range, l_range,
           h_max, h_min, k_max, k_min, l_max, l_min, dmin,
           fudge, complex_miller, verbose, only_save_omega_kahn,
           isotropic_ncells, compute_curvatures,
           FhklLinear, Fhkl2Linear,
           refine_Bmat, refine_Ncells, refine_pan_orig, refine_pan_rot,
           fcell_man->refine_me, refine_lambda, eta_man->refine_me, refine_Umat,
           fdet_vectors, sdet_vectors,
           odet_vectors, pix0_vectors,
           nopolar, point_pixel, fluence, r_e_sqr, spot_scale,
           sources, device_Id,
           device_pointers,
           update_step_positions_on_device, update_panels_fasts_slows_on_device,
           update_sources_on_device, update_umats_on_device,
           update_dB_matrices_on_device, update_rotmats_on_device,
           update_Fhkl_on_device, update_detector_on_device, update_refine_flags_on_device,
           update_panel_deriv_vecs_on_device);
    }
    gettimeofday(&t2, 0);
    double time = (1000000.0*(t2.tv_sec-t1.tv_sec) + t2.tv_usec-t1.tv_usec)/1000.0;
    if(verbose){
        int n_total_iter = Nsteps*Npix_to_model;
        if(use_cuda)
            printf("TIME TO RUN DIFFBRAGG -GPU- KERNEL (%d iterations):  %3.10f ms \n",n_total_iter, time);
        else
            printf("TIME TO RUN DIFFBRAGG -CPU-  KERNEL (%d iterations):  %3.10f ms \n",n_total_iter, time);
    }
    // TODO behold inefficient
    for (int i_pix=0; i_pix< Npix_to_model; i_pix++){
        floatimage_roi[i_pix] = image[i_pix];
        for (int i_rot=0; i_rot<3; i_rot++){
            if (rot_managers[i_rot]->refine_me){
                int idx = i_rot*Npix_to_model + i_pix;
                rot_managers[i_rot]->increment_image(i_pix, d_Umat_images[idx], d2_Umat_images[idx], compute_curvatures);
            }
        }
        for (int i_uc=0; i_uc<3; i_uc++){
            if (ucell_managers[i_uc]->refine_me){
                int idx = i_uc*Npix_to_model + i_pix;
                ucell_managers[i_uc]->increment_image(i_pix, d_Bmat_images[idx], d2_Bmat_images[idx], compute_curvatures);
            }
        }
        if (Ncells_managers[0]->refine_me){
            Ncells_managers[0]->increment_image(i_pix, d_Ncells_images[i_pix], d2_Ncells_images[i_pix], compute_curvatures);
            if (! isotropic_ncells){
                int idx= Npix_to_model+i_pix;
                Ncells_managers[1]->increment_image(i_pix, d_Ncells_images[idx], d2_Ncells_images[idx], compute_curvatures);
                idx = 2*Npix_to_model + i_pix;
                Ncells_managers[2]->increment_image(i_pix, d_Ncells_images[idx], d2_Ncells_images[idx], compute_curvatures);

            }
        }
        if (fcell_man->refine_me){
            fcell_man->increment_image(i_pix, d_fcell_images[i_pix], d2_fcell_images[i_pix], compute_curvatures);
        }

        if (eta_man->refine_me)
            eta_man->increment_image(i_pix, d_eta_images[i_pix], 0, compute_curvatures);

        for(int i_lam=0; i_lam < 2; i_lam++){
            if (lambda_managers[i_lam]->refine_me){
                int idx= Npix_to_model*i_lam + i_pix;
                lambda_managers[i_lam]->increment_image(i_pix, d_lambda_images[idx], d2_lambda_images[idx], compute_curvatures);
            }
        }

        for(int i_pan=0; i_pan <3; i_pan++){
            int i_rot = pan_rot_ids[i_pan];
            if (panels[i_rot]->refine_me){
                int idx = Npix_to_model*i_pan + i_pix;
                panels[i_rot]->increment_image(i_pix, d_panel_rot_images[idx], d2_panel_rot_images[idx], compute_curvatures);
            }

            int i_orig = pan_orig_ids[i_pan];
            if(panels[i_orig]->refine_me){
                int idx= Npix_to_model*i_pan + i_pix;
                panels[i_orig]->increment_image(i_pix, d_panel_orig_images[idx], d2_panel_orig_images[idx], compute_curvatures);
            }
        }
    } // END of flex array update

    delete[] subS_pos;
    delete[] subF_pos;
    delete[] thick_pos;
    delete[] source_pos;
    delete[] phi_pos;
    delete[] mos_pos;
    if(verbose) printf("done with pixel loop\n");
} // END  of add_diffBragg_spots


void diffBragg::diffBragg_rot_mats(){
    for (int i_rot=0; i_rot < 3; i_rot++){
        if (rot_managers[i_rot]->refine_me){
            RotMats[i_rot] = rot_managers[i_rot]->R;
            dRotMats[i_rot] = rot_managers[i_rot]->dR;
            d2RotMats[i_rot] = rot_managers[i_rot]->dR2;
            R3[i_rot] = RotMats[i_rot];
            R3_2[i_rot] = RotMats[i_rot];
        }
    }
    RXYZ = RotMats[0]*RotMats[1]*RotMats[2];
    /*  update Umats to be U*RXYZ   */
    for(mos_tic=0;mos_tic<mosaic_domains;++mos_tic){
        UMATS_RXYZ[mos_tic] = UMATS[mos_tic] * RXYZ;
        if (eta_man->refine_me){
            //if (verbose)
            //   printf("setting umat %d in vector of length %d\n" , mos_tic, UMATS_RXYZ_prime.size());
            UMATS_RXYZ_prime[mos_tic] = UMATS_prime[mos_tic]*RXYZ;
            }
    }
}

void diffBragg::diffBragg_list_steps(
                int* subS_pos,  int* subF_pos,  int* thick_pos,
                int* source_pos,  int* phi_pos,  int* mos_pos ){
    /* TODO theres probably a clever way to do this, but oh well */
    // TODO: time me
    int i_step = 0;
    for(subS=0;subS<oversample;++subS){
        for(subF=0;subF<oversample;++subF){
            for(thick_tic=0;thick_tic<detector_thicksteps;++thick_tic){
                for(source=0;source<sources;++source){
                    for(phi_tic = 0; phi_tic < phisteps; ++phi_tic){
                        for(mos_tic=0;mos_tic<mosaic_domains;++mos_tic){
                            subS_pos[i_step] = subS;
                            subF_pos[i_step] = subF;
                            thick_pos[i_step] = thick_tic;
                            source_pos[i_step] = source;
                            phi_pos[i_step] = phi_tic;
                            mos_pos[i_step] = mos_tic;
                            i_step ++;
                        }
                    }
                }
            }
        }
    }
}

void diffBragg::linearize_Fhkl(){
	int hklsize = h_range * k_range * l_range;
	FhklLinear.clear();
	Fhkl2Linear.clear();
	for (int h = 0; h < h_range; h++) {
		for (int k = 0; k < k_range; k++) {
			for (int l = 0; l < l_range; l++) {
				FhklLinear.push_back(Fhkl[h][k][l]);
				if (complex_miller)
                    Fhkl2Linear.push_back(Fhkl2[h][k][l]);
			}
		}
	}
}


} // end of namespace nanoBragg
} // end of namespace simtbx
