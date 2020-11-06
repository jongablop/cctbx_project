
#include <simtbx/nanoBragg/nanoBragg.h>
#include <vector>
#include <boost/ptr_container/ptr_vector.hpp>
#include <Eigen/Dense>
#include <boost/python.hpp>
#include<Eigen/StdVector>
//#include <boost/python/numpy.hpp>

namespace simtbx {
namespace nanoBragg {

class derivative_manager{
  public:
    virtual ~derivative_manager(){}
    derivative_manager();
    void initialize(int sdim, int fdim, bool compute_curvatures);
    af::flex_double raw_pixels;
    af::flex_double raw_pixels2;
    double value; // the value of the parameter
    double dI; // the incremental derivative
    double dI2; // the incremental derivative
    bool refine_me;
    bool second_derivatives;
    void increment_image(int idx, double value, double value2, bool curvatures);

};

class panel_manager: public derivative_manager{
  public:
    panel_manager();
   virtual void foo(){}
    virtual ~panel_manager(){}
    //virtual void set_R();
    //void increment(double Iincrement, double omega_pixel, Eigen::Matrix3d M, double pix2, Eigen::Vector3d o, Eigen::Vector3d k_diffracted,
    //            double per_k, double per_k3, double per_k5, Eigen::Vector3d V);

    Eigen::Matrix3d dR;
    Eigen::Vector3d dF;
    Eigen::Vector3d dS;
    Eigen::Vector3d dk;
    Eigen::Vector3d F_cross_dS;
    Eigen::Vector3d dF_cross_S;

}; // end of rot_manager


class rot_manager: public derivative_manager{
  public:
    rot_manager();
    virtual ~rot_manager(){}
    virtual void set_R();
    void increment(double value, double value2);
    Eigen::Matrix3d XYZ, XYZ2;
    Eigen::Matrix3d R, dR, dR2;

}; // end of rot_manager

class Ncells_manager: public derivative_manager{
  public:
    Ncells_manager();
    virtual ~Ncells_manager(){}
    void increment(double dI_increment, double dI2_increment);
    //bool single_parameter;  // refine a single parameter (Na=Nb=Nc), or else all 3 parameters (Na, Nb, Nc)
};


class Fcell_manager: public derivative_manager{
  public:
    Fcell_manager();
    virtual ~Fcell_manager(){}
    void increment(double value, double value2);
};

class eta_manager: public derivative_manager{
  public:
    eta_manager();
    virtual ~eta_manager(){}
    void increment(double value, double value2);
};

class lambda_manager: public derivative_manager{
  public:
    lambda_manager();
    virtual ~lambda_manager(){}
    void increment(double value, double value2);
    double dg_dlambda;
};


class ucell_manager: public derivative_manager{
  public:
    ucell_manager();
    virtual ~ucell_manager(){}
    void increment(double value, double value2);

    Eigen::Matrix3d dB, dB2;
};

class origin_manager: public derivative_manager{
  public:
    origin_manager();
    virtual ~origin_manager(){}
    //void increment(
    //    const Eigen::Ref<const Eigen::Vector3d>vec3 V,
    //    const Eigen::Ref<const Eigen::Matrix3d>mat3 N, const Eigen::Ref<const Eigen::Matrix3d>mat3 UBO,
    //    const Eigen::Ref<const Eigen::Vector3d>vec3 k_diffracted, const Eigen::Ref<const Eigen::Vector3d>vec3 o_vec,
    //    double air_path, double wavelen, double Hrad, double Fcell, double Flatt, double fudge,
    //    double source_I, double capture_fraction, double omega_pixel, double pixel_size);
    Eigen::Vector3d dk; /* derivative of the diffracted vector along this origin component */
    double FF, FdF, FdF2, dFdF;
};

class rotX_manager: public rot_manager{
  public:
    rotX_manager();
    void set_R();
};
class rotY_manager: public rot_manager{
  public:
    rotY_manager();
    void set_R();
};
class rotZ_manager: public rot_manager{
  public:
    rotZ_manager();
    void set_R();
};

class diffBragg: public nanoBragg{
  public:
  diffBragg(const dxtbx::model::Detector& detector, const dxtbx::model::Beam& beam,
            int verbose, int panel_id);

  ~diffBragg(){};
  void initialize_managers();
  void vectorize_umats();
  void rotate_fs_ss_vecs(double panel_rot_ang);
  void rotate_fs_ss_vecs_3D(double panel_rot_angO, double panel_rot_angF, double panel_rot_angS);
  void add_diffBragg_spots();
  void init_raw_pixels_roi();
  void zero_raw_pixel_rois();
  void set_ucell_derivative_matrix(int refine_id, af::shared<double> const& value);
  void set_ucell_second_derivative_matrix(int refine_id, af::shared<double> const& value);
  void init_Fhkl2();
  af::flex_double get_panel_increment(double Iincrement, double omega_pixel, const Eigen::Ref<const Eigen::Matrix3d>& M, double pix2,
            const Eigen::Ref<const Eigen::Vector3d>& o, const Eigen::Ref<const Eigen::Vector3d>& k_diffracted,
            double per_k, double per_k3, double per_k5, const Eigen::Ref<const Eigen::Vector3d>& V, const Eigen::Ref<const Eigen::Vector3d>& _dk);
  inline void free_Fhkl2(){
      if(Fhkl2 != NULL) {
        for (h0=0; h0<=h_range;h0++) {
          for (k0=0; k0<=k_range;k0++) {
            free(Fhkl2[h0][k0]);
          }
          free(Fhkl2[h0]);
        }
        free(Fhkl2);
      }
  }
  //void reset_derivative_pixels(int refine_id);

  /* methods for interacting with the derivative managers */
  void refine(int refine_id);
  void update_dxtbx_geoms(const dxtbx::model::Detector& detector, const dxtbx::model::Beam& beam,
        int panel_id, double panel_rot_angO=0,
        double panel_rot_angF=0,  double panel_rot_angS=0, double panel_offsetX=0,
        double panel_offsetY=0, double panel_offsetZ=0, bool force=true);
  void set_value( int refine_id, double value);
  void set_ncells_values( boost::python::tuple const& values);
  boost::python::tuple get_ncells_values();
  double get_value( int refine_id);
  af::flex_double get_derivative_pixels(int refine_id);
  af::flex_double get_second_derivative_pixels(int refine_id);
  af::flex_double get_raw_pixels_roi();
  boost::python::tuple get_ncells_derivative_pixels();
  boost::python::tuple get_ncells_second_derivative_pixels();

  boost::python::tuple get_lambda_derivative_pixels();

  /* override to cache some of the polarization calc variables to use in derivatives*/
  double polarization_factor(double kahn_factor, double *incident, double *diffracted, double *axis);

  double psi;  /* for polarization correction */
  double u;
  double kEi;
  double kBi;
  Eigen::Vector3d O_reference;

  //bool use_omega_pixel_ave;
  double om;
  double omega_pixel_ave;
  double airpath_ave;

  double diffracted_ave[4];
  double pixel_pos_ave[4];
  double Fdet_ave, Sdet_ave, Odet_ave;
  void simple_mul(double matA[9], double matB[9], double matC[9]);
  void simple_mul_right_transpose(double matA[9], double matB[9], double matC[9]);
  void simple_mul_left_transpose(double matA[9], double matB[9], double matC[9]);
  //void simple_mul( double a[3][3], double b[3][3], double c[3][3]);
  Eigen::Vector3d k_diffracted_ave;
  Eigen::Vector3d k_incident_ave;

  Eigen::Matrix3d EYE;
  mat3 Umatrix;
  mat3 Bmatrix;
  mat3 Omatrix;
  //mat3 UBO;
  //mat3 Bmat_realspace, NABC; //, dN;
  Eigen::Matrix3d NABC;
  Eigen::Matrix3d RXYZ;
  std::vector<Eigen::Matrix3d,Eigen::aligned_allocator<Eigen::Matrix3d> > RotMats, dRotMats, d2RotMats, R3, R3_2,
    UMATS, UMATS_RXYZ, UMATS_prime, UMATS_RXYZ_prime;
  //std::vector<Eigen::Matrix3d> RotMats;
  //std::vector<Eigen::Matrix3d> dRotMats, d2RotMats;
  //std::vector<Eigen::Matrix3d> R3, R3_2;

  // Panel rotation
  Eigen::Matrix3d panR;
  Eigen::Matrix3d panR2;
  double panel_rot_ang;

  //vec3 k_diffracted;
  //vec3 o_vec;
  Eigen::Vector3d Ei_vec, Bi_vec;
  //vec3 H_vec, H0_vec;
  //vec3 a_vec, ap_vec;
  //vec3 b_vec, bp_vec;
  //vec3 c_vec, cp_vec;
  //vec3 q_vec; // scattering vector

  //std::vector<Eigen::Matrix3d> UMATS;
  //std::vector<Eigen::Matrix3d> UMATS_RXYZ;
  //std::vector<Eigen::Matrix3d> UMATS_prime;
  //std::vector<Eigen::Matrix3d> UMATS_RXYZ_prime;
  double * mosaic_umats_prime;
  void set_mosaic_blocks_prime(af::shared<mat3> umat_in);  // set the individual mosaic block orientation derivatives from array
  //bool vectorized_umats;

  /* derivative managers */
  std::vector<boost::shared_ptr<rot_manager> > rot_managers;
  std::vector<boost::shared_ptr<ucell_manager> > ucell_managers;
  std::vector<boost::shared_ptr<Ncells_manager> > Ncells_managers;
  std::vector<boost::shared_ptr<origin_manager> > origin_managers;
  std::vector<boost::shared_ptr<lambda_manager> > lambda_managers;
  std::vector<boost::shared_ptr<panel_manager> > panels;
  boost::shared_ptr<Fcell_manager> fcell_man;
  boost::shared_ptr<eta_manager> eta_man;
  boost::shared_ptr<panel_manager> panel_rot_man;
  boost::shared_ptr<panel_manager> panel_rot_manF;
  boost::shared_ptr<panel_manager> panel_rot_manS;

  double* floatimage_roi;
  af::flex_double raw_pixels_roi;
  //af::flex_int raw_pixels_roi;
  //af::flex_double raw_pixels_roi;
  //af::flex_double raw_pixels_roi;

  bool compute_curvatures;
  bool update_oversample_during_refinement;
  bool oversample_omega;
  bool only_save_omega_kahn;
  double uncorrected_I;
  Eigen::Vector3d max_I_hkl;// the hkl corresponding to the maximum intensity in the array (debug)
  //int max_I_h, max_I_k, max_I_l;

  // helpful definitions..
  double per_k ;
  double per_k2 ;
  double per_k3 ;
  double per_k4 ;
  double per_k5;
  double per_k6;
  double per_k7;
  double G ;

  double du;
  double du2;

  double w ;
  double w2;
  double BperE2;
  double dkE ;
  double dkB;
  double v ;
  double dv;
  double dpsi ;
  double dpsi2;

  double c2psi ;
  double s2psi ;
  double gam_cos2psi;
  double gam_sin2psi;

  double dpolar ;

  double dpolar2;

  double pp ;
  double dOmega ;
  double dOmega2;

  double FF ;
  double FdF ;
  double dFdF ;
  double FdF2;

  /* om is the average solid angle in the pixel (average over sub pixels) */
  double origin_dI ;
  double origin_dI2;
  // different scale term here because polar and domega terms depend on originZ
  double scale_term2 ;
  double scale_term ;

  double ***Fhkl2;  // = NULL
  af::shared<double> pythony_amplitudes2;
  bool complex_miller;
  double F_cell2; // for storing the imaginary component

  bool isotropic_ncells;

  double source_lambda0, source_lambda1;
  bool use_lambda_coefficients;

 // Eigen types
 Eigen::Matrix3d eig_U, eig_B, eig_O;
}; // end of diffBragg

} // end of namespace nanoBragg
} // end of namespace simtbx
