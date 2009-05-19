#include <assert.h>
#include <math.h>
#include <iostream>
#include <cctbx/adptbx.h>
#include <cctbx/error.h>
#include <cctbx/adp_restraints/isotropic_adp.h>

namespace cctbx { namespace adp_restraints {

  //! Constructor.
  isotropic_adp::isotropic_adp(
    af::const_ref<scitbx::sym_mat3<double> > const& u_cart_,
    isotropic_adp_proxy const& proxy)
  :
    weight(proxy.weight)
  {
    for (int i=0;i<2;i++) {
      std::size_t i_seq = proxy.i_seq;
      CCTBX_ASSERT(i_seq < u_cart_.size());
      u_cart = u_cart_[i_seq];
    }
    init_deltas();
  }

  void
  isotropic_adp::init_deltas()
  {
    double const u_iso =
      adptbx::u_cart_as_u_iso(u_cart);
    for (int i=0;i<3;i++) {
      deltas_[i] = u_cart[i] - u_iso;
    }
    for (int i=3;i<6;i++) {
      deltas_[i] = u_cart[i];
    }
  }

  /* This is the square of the Frobenius norm of the matrix of deltas, or
     alternatively the inner product of the matrix of deltas with itself.
     This is used since the residual is then rotationally invariant.
   */
  double
  isotropic_adp::residual() const
  {
    return weight * deltas_.dot(deltas_);
  }

  double
  isotropic_adp::rms_deltas() const
  {
    return std::sqrt(deltas_.dot(deltas_)/9);
  }

  scitbx::sym_mat3<double>
  isotropic_adp::gradients() const
  {
    scitbx::sym_mat3<double> gradients;
    for (int i=0;i<3;i++) {
      gradients[i] = weight * 2 * deltas_[i];
    }
    for (int i=3;i<6;i++) {
      gradients[i] = weight * 4 * deltas_[i];
    }
    return gradients;
  }

  void
  isotropic_adp::add_gradients(
    af::ref<scitbx::sym_mat3<double> > const& gradients_aniso_cart,
    unsigned const& i_seq) const
  {
    gradients_aniso_cart[i_seq] += gradients();
  }

  double
  isotropic_adp_residual_sum(
  af::const_ref<scitbx::sym_mat3<double> > const& u_cart,
  af::const_ref<isotropic_adp_proxy> const& proxies,
  af::ref<scitbx::sym_mat3<double> > const& gradients_aniso_cart)
  {
    CCTBX_ASSERT(   gradients_aniso_cart.size() == 0
                 || gradients_aniso_cart.size() == u_cart.size());
    double result = 0;
    for(std::size_t i=0;i<proxies.size();i++) {
      isotropic_adp_proxy const& proxy = proxies[i];
      isotropic_adp restraint(u_cart, proxy);
      result += restraint.residual();
      if (gradients_aniso_cart.size() != 0) {
        restraint.add_gradients(gradients_aniso_cart, proxy.i_seq);
      }
    }
    return result;
  }

  af::shared<double>
  isotropic_adp_residuals(
    af::const_ref<scitbx::sym_mat3<double> > const& u_cart,
    af::const_ref<isotropic_adp_proxy> const& proxies)
  {
    af::shared<double> result((af::reserve(proxies.size())));
    for(std::size_t i=0;i<proxies.size();i++) {
      isotropic_adp_proxy const& proxy = proxies[i];
      isotropic_adp restraint(u_cart, proxy);
      result.push_back(restraint.residual());
    }
    return result;
  }

  af::shared<double>
  isotropic_adp_deltas_rms(
    af::const_ref<scitbx::sym_mat3<double> > const& u_cart,
    af::const_ref<isotropic_adp_proxy> const& proxies)
  {
    af::shared<double> result((af::reserve(proxies.size())));
    for(std::size_t i=0;i<proxies.size();i++) {
      result.push_back(isotropic_adp(u_cart, proxies[i]).rms_deltas());
    }
    return result;
  }

}} // cctbx::adp_restraints
