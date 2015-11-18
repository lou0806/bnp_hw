//[[Rcpp::depends(RcppArmadillo)]]
#include <RcppArmadillo.h>
#include <RcppArmadilloExtensions/sample.h>
#include <iostream>
#include <dp/dp.h>

using namespace std;
using namespace arma;
using namespace Rcpp ;

//[[Rcpp::export]]
vec test (int n, double a, double b) {
  vec z1 = randg(n, distr_param(a,1.0));
  vec z2 = randg(n, distr_param(b,1.0));
  return z1 / (z1+z2);
} 

// Enable C++11 in R: Sys.setenv("PKG_CXXFLAGS"="-std=c++11")
// This is used for randg(shape,scale)

// Functions for this assignment
double update_mu (double mu, double tau, vec theta, double a, double b, double cs,
    int* acc) {
  // [mu] [prod_{t=uniqueThetas} g_0(t|mu)]
  vec ut = unique( theta );
  int n = ut.size();
  double lg, lg1, lg2, c, mu_new;

  mu_new = mu;
  c = randn() * cs + mu; // c=candidate, cs = candidiate sigma

  if (c > 0) {
    lg1 = (a-1) * log(mu) - b*mu + n*mu*log(tau) - n*lgamma(mu) + mu*sum(log(ut));
    lg2 = (a-1) * log(c) - b*mu + n*c*log(tau) - n*lgamma(c) + c*sum(log(ut));

    if ( lg > log(randu()) ) {
      mu_new = c;
      *acc++;
    }
  }

  return mu_new;
  //return 1.0;
}

double rand_beta (double a, double b) { // Only for one draw. Write a function for vectors.
  double z1 = randg(1, distr_param(a,1.0))[0];
  double z2 = randg(1, distr_param(b,1.0))[0];
  return z1 / (z1+z2);
}

double update_tau (double mu, vec theta, double a, double b) {
  // [tau] [prod_{t=uniqueThetas} g_0(t|tau)]
  vec ut = unique( theta );
  int n = ut.size();
  return randg(1, distr_param(a + mu * n, 1 / (b + sum(ut))) )[0]; // shape, sc
}

double update_alpha (double alpha, vec theta, double a, double b, int n) {
  double eta, c, d;
  int ns, ind;
  double alpha_new;
  vec ut = unique(theta);

  eta = rand_beta( alpha + 1, n );
  ns = ut.size();
  c = a + ns;
  d = b - log(eta);
  
  ind = wsample({0,1}, {c-1, n*d});
  if (ind == 0) {
    alpha_new = randg(1, distr_param(c, 1/d) )[0]; // shape, rate
  } else {
    alpha_new = randg(1, distr_param(c-1, 1/d) )[0];
  }

  return alpha_new;
}

double update_beta (double beta, vec theta, double m, double s, vec y, vec x, 
    double cs, int* acc) {
    // [beta[ [prod_{i=1:n} (k(y_i|t_i,beta) ]

  double lg1, lg2, lg, c, beta_new;

  beta_new = beta;
  c = randn() * cs + beta;

  lg1 = -pow((beta-m)/s,2) / 2 + sum(beta * y % log(theta) % x - theta % exp(beta*x));
  lg2 = -pow((c-m)/s,2) / 2 + sum(c * y % log(theta) % x - theta % exp(c*x));
  lg = lg1 - lg2;

  if (lg > log(randu()) ) {
    beta_new = c;
    *acc++;
  }

  return beta_new;
}

vec update_theta (vec theta, double mu, double tau, double beta, double alpha, vec x, vec y) {
  vec theta_new;


  return theta_new;
}

//[[Rcpp::export]]
List dppois(vec y, vec x, double a_mu, double b_mu, double a_tau, double b_tau,
    double a_alpha, double b_alpha, double m_beta, double s_beta, 
    double cs_mu, double cs_beta, int B) { 
  // also need cand_sigs
  // all b_* are rates in the gamma distribution

  int acc_mu=0, acc_beta=0, n=y.size();
  mat theta = zeros<mat>(B,n);
  vec mu, tau, beta, alpha, tb, acc_theta=zeros<vec>(n);
  mu = tau = beta = alpha = ones<vec>(B);
  List ret;

  // Start MCMC
  for (int b=1; b<B; b++) {
    // Update thetas
    //theta.row(b) = update_theta(theta.row(b-1),);
    tb = vectorise( theta.row(b) );

    beta[b] = update_beta(beta[b-1], tb, m_beta, s_beta, y, x, cs_beta, &acc_beta);
    mu[b] = update_mu(mu[b-1], tau[b-1], tb, a_mu, b_mu, cs_mu, &acc_mu);
    tau[b] = update_tau (mu[b], tb, a_tau, b_tau);
    alpha[b] = update_alpha(alpha[b], tb, a_alpha, b_alpha, n);

    cout << "\r" << b;
  } // End of MCMC

  ret["mu"] = mu;
  ret["tau"] = tau;
  ret["beta"] = beta;
  ret["alpha"] = alpha;
  ret["theta"] = theta;
  ret["acc_mu"] = acc_mu;
  ret["acc_beta"] = acc_beta;
  ret["acc_theta"] = acc_theta;

  return ret;
}

// Need a posterior predictive function
