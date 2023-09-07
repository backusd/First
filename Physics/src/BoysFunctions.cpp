#include "BoysFunctions.h"

#include "MathUtils.h"
#include "BoysFunction.h"

namespace GaussianIntegrals {


	void BoysFunctions::GenerateBoysFunctions(int maxM, double T)
	{
		functions.resize(maxM + 1ULL);

		BoysFunction boysFunction;
		functions[maxM] = boysFunction(maxM, T);

		for (int m = maxM - 1; m >= 0; --m)
		{
			functions[m] = (2. * T * functions[m + 1ULL] + exp(-T)) / (2. * m + 1.);

			//functions[m] = boysFunction(m, T);			

			//TRACE(L"Boys function m=%d: %f\n", m, functions[m]);
		}
	}


}