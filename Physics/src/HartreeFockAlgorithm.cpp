#include "HartreeFockAlgorithm.h"
#include "Constants.h"


namespace HartreeFock {

	HartreeFockAlgorithm::HartreeFockAlgorithm(int iterations)
		: totalEnergy(std::numeric_limits<double>::infinity()),
		mp2Energy(0),
		nuclearRepulsionEnergy(0),
		nuclearElectricFieldEnergy(0),
		numberOfOrbitals(0),
		maxIterations(iterations),
		inited(false),
		alpha(0.75),
		initGuess(0.75),
		terminate(false),
		converged(false),
		HOMOEnergy(0),
		lastErrorEst(0),
		UseDIIS(true),
		maxDIISiterations(1000),
		normalIterAfterDIIS(500)
	{

	}
	void HartreeFockAlgorithm::PrintBase()
	{
//		TRACE("HartreeFockAlgorithm: alpha                      = %f\n", alpha);
//		TRACE("HartreeFockAlgorithm: converged                  = %s\n", converged ? "true" : "false");
//		TRACE("HartreeFockAlgorithm: HOMOEnergy                 = %f\n", HOMOEnergy);
//		TRACE("HartreeFockAlgorithm: inited                     = %s\n", inited ? "true" : "false");
//		TRACE("HartreeFockAlgorithm: initGuess                  = %f\n", initGuess);
//		TRACE("HartreeFockAlgorithm: lastErrorEst               = %f\n", lastErrorEst);
//		TRACE("HartreeFockAlgorithm: maxDIISiterations          = %d\n", maxDIISiterations);
//		TRACE("HartreeFockAlgorithm: maxIterations              = %d\n", maxIterations);
//		TRACE("HartreeFockAlgorithm: mp2Energy                  = %f\n", mp2Energy);
//		TRACE("HartreeFockAlgorithm: normalIterAfterDIIS        = %d\n", normalIterAfterDIIS);
//		TRACE("HartreeFockAlgorithm: nuclearElectricFieldEnergy = %f\n", nuclearElectricFieldEnergy);
//		TRACE("HartreeFockAlgorithm: nuclearRepulsionEnergy     = %f\n", nuclearRepulsionEnergy);
//		TRACE("HartreeFockAlgorithm: numberOfOrbitals           = %d\n", numberOfOrbitals);
//		TRACE("HartreeFockAlgorithm: terminate                  = %s\n", terminate ? "true" : "false");
//		TRACE("HartreeFockAlgorithm: totalEnergy                = %f\n", totalEnergy);
//		TRACE("HartreeFockAlgorithm: UseDIIS                    = %s\n", UseDIIS ? "true" : "false");
	}
	void HartreeFockAlgorithm::Print()
	{
		PrintBase();
	}


	HartreeFockAlgorithm::~HartreeFockAlgorithm()
	{
	}


	void HartreeFockAlgorithm::Init(Systems::Molecule* molecule, bool debug)
	{
		converged = false;
		integralsRepository.Reset(molecule);

		overlapMatrix.SetRepository(&integralsRepository);
		momentMatrix.SetRepository(&integralsRepository);
		kineticMatrix.SetRepository(&integralsRepository);
		nuclearMatrix.SetRepository(&integralsRepository);

//		if (debug)
//			TRACE("Initializing overlap matrix...\n");

		overlapMatrix.Calculate();
		momentMatrix.Calculate();
		kineticMatrix.Calculate();
		nuclearMatrix.Calculate();

//		if (debug)
//		{
//			std::ostringstream oss_overlap;
//			oss_overlap << overlapMatrix.matrix;
//			TRACE("\nOverlap Matrix:\n%s\n", oss_overlap.str().c_str());
//
//			std::ostringstream oss_moment;
//			oss_moment << momentMatrix.matrix;
//			TRACE("\nMoment Matrix:\n%s\n", oss_moment.str().c_str());
//
//			std::ostringstream oss_kinetic;
//			oss_kinetic << kineticMatrix.matrix;
//			TRACE("\nKinetic Matrix:\n%s\n", oss_kinetic.str().c_str());
//
//			std::ostringstream oss_nuclear;
//			oss_nuclear << nuclearMatrix.matrix;
//			TRACE("\nNuclear Matrix:\n%s\n", oss_nuclear.str().c_str());
//		}

		// Print out each of the matrices
		// Why do we call ClearMatricesMaps next?

		integralsRepository.ClearMatricesMaps();

		h = kineticMatrix.matrix + nuclearMatrix.matrix + molecule->ElectricField.X * momentMatrix.matrix + molecule->ElectricField.Y * momentMatrix.matrixY + molecule->ElectricField.Z * momentMatrix.matrixZ;

		nuclearRepulsionEnergy = molecule->NuclearRepulsionEnergy();
		nuclearElectricFieldEnergy = molecule->NuclearElectricFieldEnergy();

		numberOfOrbitals = molecule->CountNumberOfContractedGaussians();

		integralsRepository.CalculateElectronElectronIntegrals(debug);
		integralsRepository.ClearAllMaps();

		Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(overlapMatrix.matrix);

		U = solver.eigenvectors();
		s = solver.eigenvalues().cwiseInverse().cwiseSqrt().asDiagonal();

		//V = U * s;
		V = U * s * U.adjoint(); // This is S^-1/2 in the original basis

		Vt = V.adjoint(); // V is actually symmetric, so this is unnecessary, Vt = V would do (not in the case V = U * s, though)

		// now, a trick that might get rid of some numerical errors from the solver, for the symmetric case (comment it if using the other one)
		V = Vt = (V + Vt) * 0.5; // now it's really symmetrical

		// what will happen is that we'll work in a 'molecular orbitals' basis, not in the original 'atomic orbitals' one
		// the new basis is orthonormal, that is, the overlap is I, the identity matrix

		// the transformed S (overlap matrix) in the new 'molecular orbitals' basis is
		// S^-1/2 * S * S^1/2 = I
		// so the generalized eigenvalue problem becomes the regular eigenvalue problem
		// see the RestrictedHartreeFock::Step for how it's done (with comments)
		// the Test class allows you to verify this

		// U is obviously unitary by construction 
		// it's the matrix with eigenvectors as columns, multiplying U * U^t just gets I
		// the matrix elements being the scalar product of the vectors, which makes it is obviously zero everywhere (different vectors are orthogonal) except the diagonal, which gets 1, that is, the norm, the scalar product of a vector with itself
		// due of the orthonormality

		// if you would do the transform with U^t * S * U, you would simply diagonalize S, obviously having the eigenvalues on the diagonal
		// we want more than that, we want to get I, so we multiply left and right with s (S^-1/2 in the diagonal basis)
		// we end up with a multiplication of three matrices, left and right having 1/sqrt(eigenvalue) on the diagonal and the middle one with eigenvalues on the diagonal
		// obviously that gets 1 on the diagonal and 0 elsewhere

		// I = s * U^t * S * U * s
		// we could simply call V = U * s and work like that 

		// a not so old version of the program worked with that, but in order to have intermediate results comparable with some results given here:
		// https://github.com/CrawfordGroup/ProgrammingProjects
		// I switched to the current variant, which also has some other nice property, see below:

		// since the identity is diagonal 1 in any basis, we can transform it back to original basis
		// I = U * I * U^t = U * s * U^t * S * U * s * U^t
		// and now we have I = Vt * S * V
		// where V = U * s * U^t and Vt = (U * s * U^t)^t = (U^t)^t * s^t * U^t = U * s * U^t = V
		// so now V should be actually symmetric (V = Vt), check it out with the Test class

		// for the case V = U * s though, things are not so nice (but can still work fine):
		// I = s * U^t * S * U * s = Vt * S * V
		// V = U * s and Vt = (U * s)^t = s * U^t which is not the same as V 

		// Note that obviously the 'molecular orbitals' basis would be different if using V = U * s
		// but still orthonormal and things should go fine in that one, too

		inited = true;
	}



	double HartreeFockAlgorithm::Calculate(bool debug)
	{
		double curEnergy = 0;
		double prevEnergy = std::numeric_limits<double>::infinity();

		if (!inited) return prevEnergy;

		int iter = 0;
		FirstIterations(iter, curEnergy, prevEnergy, debug);
		if (terminate) return curEnergy;

		// did it converge with DIIS?
		if (UseDIIS && iter < maxDIISiterations && converged && normalIterAfterDIIS)
		{
			SelfConsistentIterations(iter, curEnergy, prevEnergy);
			if (terminate) return curEnergy;
		}

		// now continue with normal iteration with convergence checking
		NormalIterations(iter, curEnergy, prevEnergy);

		return curEnergy;
	}

	void HartreeFockAlgorithm::FirstIterations(int& iter, double& curEnergy, double& prevEnergy, bool debug)
	{
		for (; iter < maxIterations; ++iter)
		{
			const double rmsD = Step(iter, debug);

			curEnergy = GetTotalEnergy();

			if (abs(prevEnergy - curEnergy) <= (UseDIIS ? energyConvergenceDIIS : energyConvergence) && rmsD < rmsDConvergence && lastErrorEst < diisConvergence) {
				converged = true;
				break;
			}

			if (terminate) return;

			prevEnergy = curEnergy;
		}
	}


	void HartreeFockAlgorithm::SelfConsistentIterations(int& iter, double& curEnergy, double& prevEnergy)
	{
		UseDIIS = false;

		// do a loop without checking the convergence, sometimes DIIS gets stuck in a bad position close to the minimum
		for (int i = 0; i < normalIterAfterDIIS; ++i)
		{
			Step(iter + i);

			curEnergy = GetTotalEnergy();
			if (terminate)
			{
				UseDIIS = true; // restore it back
				return;
			}
			prevEnergy = curEnergy;
		}

		iter += normalIterAfterDIIS;

		UseDIIS = true; // restore it back
	}

	void HartreeFockAlgorithm::NormalIterations(int& iter, double& curEnergy, double& prevEnergy)
	{
		if (!converged)
		{
			for (; iter < maxIterations; ++iter)
			{
				const double rmsD = Step(iter);

				curEnergy = GetTotalEnergy();

				if (abs(prevEnergy - curEnergy) <= energyConvergence && rmsD < rmsDConvergence) {
					converged = true;
					return;
				}

				if (terminate) return;

				prevEnergy = curEnergy;
			}
		}
	}


	double HartreeFockAlgorithm::DiffDensityMatrices(const Eigen::MatrixXd& oldP, const Eigen::MatrixXd& newP)
	{
		double res = 0.0;

		assert(oldP.cols() == newP.cols());
		assert(oldP.rows() == newP.rows());

		for (int i = 0; i < oldP.rows(); ++i)
			for (int j = 0; j < oldP.cols(); ++j)
			{
				const double val = oldP(i, j) - newP(i, j);

				res += val * val;
			}

		return sqrt(res);
	}


	void HartreeFockAlgorithm::NormalizeC(Eigen::MatrixXd& C, const std::vector<bool>& occupied)
	{
		assert(occupied.size() <= static_cast<size_t>(C.cols()));
		assert(C.rows() == overlapMatrix.matrix.rows());

		for (int vec = 0; vec < occupied.size(); ++vec)
		{
			if (!occupied[vec]) continue;

			double factor = 0.0;

			for (int i = 0; i < overlapMatrix.matrix.rows(); ++i)
				for (int j = 0; j < overlapMatrix.matrix.cols(); ++j)
					factor += C(i, vec) * overlapMatrix.matrix(i, j) * C(j, vec);

			C.col(vec) /= sqrt(factor);
		}
	}


	Vector3D<double> HartreeFockAlgorithm::GetNuclearMoment() const
	{
		Vector3D<double> moment;

		for (const Systems::Atom& atom : integralsRepository.getMolecule()->atoms)
			moment += static_cast<double>(atom.Z) * atom.position;

		return moment;
	}


}








