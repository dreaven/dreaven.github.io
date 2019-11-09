import numpy as np
import matplotlib.pyplot as plt
import math
from math import exp, sqrt
from scipy.special import erf

def get_truncated_laplacian_l1_cost(eps, delta, D=1.0):
  t = 1.0 + (math.exp(eps)-1)/2.0/delta
  A = D/eps * math.log(t)
  B = 0.5/D*eps/(1.0-1.0/t)
  lam = D/eps
 
  t2 = math.exp(-A/lam)
  
  res = 2*B*lam * (-A*t2 + lam*(1-t2))
  
  return res

def get_truncated_laplacian_l2_cost(eps, delta, D=1.0):
  t = 1.0 + (math.exp(eps)-1)/2.0/delta
  logt = math.log(t)

  res = 2*D*D/eps/eps*(1 -(0.5 *logt *logt + logt)/(t-1))
  
  return res


# The function below is borrowed from
# https://github.com/BorjaBalle/analytic-gaussian-mechanism/blob/master/agm-example.py
def calibrateAnalyticGaussianMechanism(epsilon, delta, GS, tol = 1.e-12):
    """ Calibrate a Gaussian perturbation for differential privacy using the analytic Gaussian mechanism of [Balle and Wang, ICML'18]
    Arguments:
    epsilon : target epsilon (epsilon > 0)
    delta : target delta (0 < delta < 1)
    GS : upper bound on L2 global sensitivity (GS >= 0)
    tol : error tolerance for binary search (tol > 0)
    Output:
    sigma : standard deviation of Gaussian noise needed to achieve (epsilon,delta)-DP under global sensitivity GS
    """

    def Phi(t):
        return 0.5*(1.0 + erf(float(t)/sqrt(2.0)))

    def caseA(epsilon,s):
        return Phi(sqrt(epsilon*s)) - exp(epsilon)*Phi(-sqrt(epsilon*(s+2.0)))

    def caseB(epsilon,s):
        return Phi(-sqrt(epsilon*s)) - exp(epsilon)*Phi(-sqrt(epsilon*(s+2.0)))

    def doubling_trick(predicate_stop, s_inf, s_sup):
        while(not predicate_stop(s_sup)):
            s_inf = s_sup
            s_sup = 2.0*s_inf
        return s_inf, s_sup

    def binary_search(predicate_stop, predicate_left, s_inf, s_sup):
        s_mid = s_inf + (s_sup-s_inf)/2.0
        while(not predicate_stop(s_mid)):
            if (predicate_left(s_mid)):
                s_sup = s_mid
            else:
                s_inf = s_mid
            s_mid = s_inf + (s_sup-s_inf)/2.0
        return s_mid

    delta_thr = caseA(epsilon, 0.0)

    if (delta == delta_thr):
        alpha = 1.0

    else:
        if (delta > delta_thr):
            predicate_stop_DT = lambda s : caseA(epsilon, s) >= delta
            function_s_to_delta = lambda s : caseA(epsilon, s)
            predicate_left_BS = lambda s : function_s_to_delta(s) > delta
            function_s_to_alpha = lambda s : sqrt(1.0 + s/2.0) - sqrt(s/2.0)

        else:
            predicate_stop_DT = lambda s : caseB(epsilon, s) <= delta
            function_s_to_delta = lambda s : caseB(epsilon, s)
            predicate_left_BS = lambda s : function_s_to_delta(s) < delta
            function_s_to_alpha = lambda s : sqrt(1.0 + s/2.0) + sqrt(s/2.0)

        predicate_stop_BS = lambda s : abs(function_s_to_delta(s) - delta) <= tol

        s_inf, s_sup = doubling_trick(predicate_stop_DT, 0.0, 1.0)
        s_final = binary_search(predicate_stop_BS, predicate_left_BS, s_inf, s_sup)
        alpha = function_s_to_alpha(s_final)
        sigma = alpha*GS/sqrt(2.0*epsilon)

    return sigma



# Plot the noise amplitude comparison
LOW_X = 0.0001
HIGH_X = 10
LOW_Y = 0.000001
HIGH_Y = 0.1

NUM_POINTS = 200

X = np.arange(LOW_X, HIGH_X, (HIGH_X-LOW_X)/NUM_POINTS)
Y = np.arange(LOW_Y, HIGH_Y, (HIGH_Y - LOW_Y)/NUM_POINTS)
X, Y = np.meshgrid(X, Y)

Z = np.zeros(X.shape)

nrow,ncol = X.shape
for i in range(0, nrow):
  for j in range(0, ncol):
    eps=X[i,j]
    delta=Y[i,j]
    Z[i,j] = get_truncated_laplacian_l1_cost(eps, delta)/ calibrateAnalyticGaussianMechanism(eps, delta, GS=1.0)


fig = plt.figure(figsize=plt.figaspect(0.4))

ax = fig.add_subplot(1, 1, 1, projection='3d')
surf = ax.plot_surface(X, Y, Z, rstride=1, cstride=1, cmap=cm.coolwarm,
                       linewidth=0, antialiased=False)
ax.set_zlim(0, 1.0)
plt.xlabel(r'$\epsilon$', fontsize=18)
plt.ylabel('$\delta$', fontsize=18)
fig.colorbar(surf, shrink=0.8, aspect=20)
plt.show()



# Plot the noise power comparison
nrow,ncol = X.shape
for i in range(0, nrow):
  for j in range(0, ncol):
    eps=X[i,j]
    delta=Y[i,j]
    gauss_cost = calibrateAnalyticGaussianMechanism(eps, delta, GS=1.0)
    gauss_cost = gauss_cost * gauss_cost
    Z[i,j] = get_truncated_laplacian_l2_cost(eps, delta)/ gauss_cost

# set up a figure twice as wide as it is tall
fig = plt.figure(figsize=plt.figaspect(0.4))
ax = fig.add_subplot(1, 1, 1, projection='3d')
surf = ax.plot_surface(X, Y, Z, rstride=1, cstride=1, cmap=cm.coolwarm,
                       linewidth=0, antialiased=False)
ax.set_zlim(0, 1.0)
plt.xlabel(r'$\epsilon$', fontsize=18)
plt.ylabel('$\delta$', fontsize=18)
fig.colorbar(surf, shrink=0.8, aspect=20)
plt.show()