#include "RestrictedHartreeFock.h"

namespace HartreeFock {


	RestrictedHartreeFock::RestrictedHartreeFock(int iterations)
		: HartreeFockAlgorithm(iterations), nrOccupiedLevels(0)
	{
	}


	RestrictedHartreeFock::~RestrictedHartreeFock()
	{
	}

	void RestrictedHartreeFock::Print()
	{
//		this->PrintBase();
//
//		TRACE("RestrictedHartreeFock: nrOccupiedLevels         = %d\n", nrOccupiedLevels);
//
	}


	void RestrictedHartreeFock::Init(Systems::Molecule* molecule, bool debug)
	{
		HartreeFockAlgorithm::Init(molecule, debug);

		DensityMatrix = Eigen::MatrixXd::Zero(h.rows(), h.cols());

		occupied.resize(0); // just in case it was resized before
		nrOccupiedLevels = molecule->ElectronsNumber() / 2;

		// this should not happen for a well behaving molecule for the restricted method, but someone might try calculate the Hydrogen atom with it
		// prevent a crash with this
		if (!nrOccupiedLevels) nrOccupiedLevels = 1;
		if (nrOccupiedLevels > static_cast<unsigned int>(numberOfOrbitals)) nrOccupiedLevels = numberOfOrbitals;

		occupied.resize(nrOccupiedLevels, true);
	}

	bool RestrictedHartreeFock::DIISStep(int iter, Eigen::MatrixXd& FockMatrix)
	{
		bool UsedDIIS = false;

		if (UseDIIS && iter && iter < maxDIISiterations)
		{
			// the density matrix should commute with the Fock matrix. The difference is the error.
			// another variant could be to subtract from the current Fock matrix the previous one to get an error estimate
			const Eigen::MatrixXd errorMatrix = overlapMatrix.matrix * DensityMatrix * FockMatrix - FockMatrix * DensityMatrix * overlapMatrix.matrix;

			// another choice: if reached the limit of kept matrices, replace the ones with the bigger error

			if (diis.AddValueAndError(FockMatrix, errorMatrix))
			{
				// use DIIS
				lastErrorEst = diis.Estimate(FockMatrix);

				UsedDIIS = true;
			}
		}
		else lastErrorEst = 0;

		return UsedDIIS;
	}


	double RestrictedHartreeFock::Step(int iter, bool debug)
	{
		// *****************************************************************************************************************

		// the Fock matrix
		Eigen::MatrixXd FockMatrix;

		InitFockMatrix(iter, FockMatrix, debug);

		// will be used for DIIS

		const bool UsedDIIS = DIISStep(iter, FockMatrix);

		// ***************************************************************************************************************************

		// solve the Roothaan-Hall equation

		//diagonalize it - it can be done faster by diagonalizing the overlap matrix outside the loop but for tests this should be enough
		//I leave it here just in case - if all that S, U, s, V seems confusing this should help :)
		//Eigen::GeneralizedSelfAdjointEigenSolver<Eigen::MatrixXd> es(FockMatrix, overlapMatrix.matrix);
		//const Eigen::MatrixXd& C = es.eigenvectors();


		// this hopefully :) is faster than the one commented above:


		// this is how it's working:

		// the original generalized eigenvalue problem is:
		// FockMatrix * C = S * C * eigenvalsDiag  
		// where C is obviously the matrix with the eigenvectors as columns, S is the overlap matrix

		// insert S^-1/2 * S^1/2:
		// FockMatrix * S^-1/2 * S^1/2 * C = S * S^-1/2 * S^1/2 * C * eigenvalsDiag

		// multiply to the left with S^1/2:
		// S^1/2 * FockMatrix * S^-1/2 * S^1/2 * C = S^1/2 * S * S^-1/2 * S^1/2 * C * eigenvalsDiag
		// but S^1/2 * FockMatrix * S^-1/2 = Vt * FockMatrix * V = FockTransformed and S^1/2 * S * S^-1/2 = I

		// so we have:
		// FockTransformed * S^1/2 * C = S^1/2 * C * eigenvalsDiag
		// call S^1/2 * C = Cprime

		// now we're left with the regular eigenvalue problem
		// FockTransformed * Cprime = Cprime * eigenvalsDiag
		// to get C in the original basis, that is, the AO one, just multiply Cprime to the left with S^-1/2 (called V in the code)
		// S^-1/2 * Cprime = S^-1/2 * S^1/2 * C = I * C = C


		// so, we end up with the rules:

		// transform an operator from the non-orthogonal AO basis into the orthonormal one:
		// Otransformed = Vt * O * V
		// transform a vector using:
		// Xtransformed = Vt * X

		// obviously you can do the inverse transform, from the orthonormal MO basis back to the original AO basis easily:
		// X = V * Xtransformed (just multiply the above one to the left with V)
		// O = V * Otransformed * Vt (again, with multiplication to the left and right)

		const Eigen::MatrixXd FockTransformed = Vt * FockMatrix * V; // orthogonalize
		const Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(FockTransformed);

		eigenvals = es.eigenvalues();
		const Eigen::MatrixXd& Cprime = es.eigenvectors();


		C = V * Cprime; // transform back the eigenvectors into the original non-orthogonalized AO basis

		// normalize it - in some rare cases it seems to help
		//NormalizeC(C, occupied);

		//***************************************************************************************************************

		// calculate the density matrix

		Eigen::MatrixXd newDensityMatrix = Eigen::MatrixXd::Zero(h.rows(), h.cols());

		for (int i = 0; i < h.rows(); ++i)
			for (int j = 0; j < h.cols(); ++j)
				for (unsigned int vec = 0; vec < occupied.size(); ++vec) // only eigenstates that are occupied 
					if (occupied[vec]) newDensityMatrix(i, j) += 2. * C(i, vec) * C(j, vec); // 2 is for the number of electrons in the eigenstate, it's the restricted Hartree-Fock


		//**************************************************************************************************************

		CalculateEnergy(eigenvals, newDensityMatrix/*, FockMatrix*/);

		//		TRACE("Step: %d Energy: %f\n", iter, totalEnergy);

				// calculate rms for differences between new and old density matrices, it can be used to check for convergence, too
		const Eigen::MatrixXd rmsDensityMatricesDif = newDensityMatrix - DensityMatrix;
		const double rmsD = sqrt(rmsDensityMatricesDif.cwiseProduct(rmsDensityMatricesDif).sum());

		// ***************************************************************************************************
		// go to the next density matrix

		if (UsedDIIS)
			DensityMatrix = newDensityMatrix;
		else
			DensityMatrix = alpha * newDensityMatrix + (1. - alpha) * DensityMatrix;  // use mixing if alpha is set less than 1

		//LastMOFockMatrix = FockTransformed;

		return rmsD;
	}

	void RestrictedHartreeFock::InitFockMatrix(int iter, Eigen::MatrixXd& FockMatrix, bool debug) const
	{
		// this could be made faster knowing that the matrix should be symmetric
		// but it would be less expressive so I'll let it as it is
		// maybe I'll improve it later
		// anyway, the slower part is dealing with electron-electron integrals

		if (0 == iter)
		{
			if (initGuess > 0)
			{
				FockMatrix.resize(h.rows(), h.cols());

				for (int i = 0; i < h.rows(); ++i)
					for (int j = 0; j < h.rows(); ++j)
						FockMatrix(i, j) = initGuess * overlapMatrix.matrix(i, j) * (h(i, i) + h(j, j)) * 0.5;
			}
			else FockMatrix = h;
		}
		else
		{
			Eigen::MatrixXd G = Eigen::MatrixXd::Zero(h.rows(), h.cols());

			for (int i = 0; i < numberOfOrbitals; ++i)
				for (int j = 0; j < numberOfOrbitals; ++j)
					for (int k = 0; k < numberOfOrbitals; ++k)
						for (int l = 0; l < numberOfOrbitals; ++l)
						{
							const double coulomb = integralsRepository.getElectronElectron(i, j, k, l);
							const double exchange = integralsRepository.getElectronElectron(i, l, k, j); // you may see it in other forms, that is, other order for indexes, but don't forget about symmetries

							G(i, j) += DensityMatrix(k, l) * (coulomb - 0.5 * exchange);

//							if (debug && iter == 1)
//							{
//								TRACE("(i, j, k, l) = (%d, %d, %d, %d)\n", i, j, k, l);
//								TRACE("    coulomb  = %f\n", coulomb);
//								TRACE("    exchange = %f\n", exchange);
//							}
						}

			FockMatrix = h + G;
		}

		// let's be sure it's symmetric
		FockMatrix = (FockMatrix + FockMatrix.transpose()) * 0.5;
	}

	void RestrictedHartreeFock::CalculateEnergy(const Eigen::VectorXd& eigenvalues, const Eigen::MatrixXd& calcDensityMatrix/*, Eigen::MatrixXd& F*/)
	{
		// one way of calculating the energy

		totalEnergy = 0;

		// simply adding energy levels does not work, because of over counting interaction energy

		for (int i = 1; i < h.rows(); ++i)
			for (int j = 0; j < i; ++j)
				totalEnergy += calcDensityMatrix(i, j) * h(j, i);

		// only the values below the diagonal were added
		// *2 to have the sum of all elements except those on diagonal
		totalEnergy *= 2.;

		// now add the diagonal elements, too
		for (int i = 0; i < h.rows(); ++i) totalEnergy += calcDensityMatrix(i, i) * h(i, i);

		totalEnergy *= 0.5;

		for (unsigned int level = 0; level < occupied.size(); ++level)
			if (occupied[level]) totalEnergy += eigenvalues(level);

		HOMOEnergy = eigenvalues(nrOccupiedLevels - 1);



		// the above is equivalent with this commented code, but since we already have eigenvalues for F calculated, we can get rid of the matrix addition to obtain H
/*
		Eigen::MatrixXd H = h + F;

		for (int i = 0; i < H.rows(); ++i)
			for (int j = 0; j < H.cols(); ++j)
				totalEnergy += calcDensityMatrix(i, j) * H(j, i);

		totalEnergy /= 2.;
*/

		totalEnergy += nuclearRepulsionEnergy/* + nuclearElectricFieldEnergy*/;
	}

	double RestrictedHartreeFock::CalculateMp2Energy()
	{
		mp2Energy = 0;

		GaussianIntegrals::MolecularOrbitalsIntegralsRepository MP2repo(integralsRepository);

		for (int i = 0; i < numberOfOrbitals; ++i)
		{
			if (i >= occupied.size() || !occupied[i]) continue; // only occupied

			for (int j = 0; j < numberOfOrbitals; ++j)
			{
				if (j >= occupied.size() || !occupied[j]) continue; // only occupied

				for (int a = 0; a < numberOfOrbitals; ++a)
				{
					if (a < occupied.size() && occupied[a]) continue; // only unoccupied

					for (int b = 0; b < numberOfOrbitals; ++b)
					{
						if (b < occupied.size() && occupied[b]) continue;  // only unoccupied

						const double Esumdif = eigenvals(i) + eigenvals(j) - eigenvals(a) - eigenvals(b);

						const double eeiajb = MP2repo.getElectronElectron(i, a, j, b, C);
						const double eeibja = MP2repo.getElectronElectron(i, b, j, a, C);

						const double partE = eeiajb * (2. * eeiajb - eeibja) / Esumdif;

						mp2Energy += partE;
					}
				}
			}
		}


		return mp2Energy;
	}


	double RestrictedHartreeFock::CalculateAtomicCharge(int atom) const
	{
		if (!integralsRepository.m_Molecule || integralsRepository.m_Molecule->atoms.size() <= atom) return 0;

		double result = integralsRepository.m_Molecule->atoms[atom].Z;

		int orbLowLimit = 0;
		int orbHighLimit = 0;
		for (int i = 0; i < integralsRepository.m_Molecule->atoms.size(); ++i)
		{
			const int numBasisFunctions = integralsRepository.m_Molecule->atoms[i].CountNumberOfContractedGaussians();
			if (i == atom)
			{
				orbHighLimit = orbLowLimit + numBasisFunctions;
				break;
			}
			orbLowLimit += numBasisFunctions;
		}

		// this is not so efficient, being done for each atom if needed instead of once for all atoms, but it's ok
		const Eigen::MatrixXd DS = DensityMatrix * overlapMatrix.matrix;

		for (int i = orbLowLimit; i < orbHighLimit; ++i)
			result -= DS(i, i);

		return result;
	}

	Vector3D<double> RestrictedHartreeFock::GetMoment() const
	{
		Vector3D<double> moment = GetNuclearMoment();

		const unsigned long long int sz = DensityMatrix.cols();

		for (unsigned int i = 0; i < sz; ++i)
			for (unsigned int j = 0; j < sz; ++j)
			{
				const double dval = DensityMatrix(i, j);
				moment.X -= dval * momentMatrix.matrix(i, j);
				moment.Y -= dval * momentMatrix.matrixY(i, j);
				moment.Z -= dval * momentMatrix.matrixZ(i, j);
			}

		return moment;
	}


}