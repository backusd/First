#pragma once
#include "pch.h"
#include "Vector3D.h"
#include "PrimitiveShell.h"

namespace Systems
{
class Atom
{
public:
	Vector3D<double> position;

	unsigned int Z;
	unsigned int electronsNumber;

	unsigned int ID;

	Atom(unsigned int nrZ = 0, int nrElectrons = -1);
	virtual ~Atom();
};

class AtomWithShells : public Atom
{
public:
	std::vector<Orbitals::ContractedGaussianShell> shells;

//	void Print()
//	{
//		TRACE("AtomWithShells: Z               = %d\n", Z);
//		TRACE("AtomWithShells: ID              = %d\n", ID);
//		TRACE("AtomWithShells: electronsNumber = %d\n", electronsNumber);
//		TRACE("AtomWithShells: position        = <%f, %f, %f>\n", position.X, position.Y, position.Z);
//		TRACE("AtomWithShells: ContractedGaussianShells -> count = %d\n", shells.size());
//		for (unsigned int iii = 0; iii < shells.size(); ++iii)
//		{
//			TRACE("    ContractedGaussianShell #%d\n", iii);
//			TRACE("        ID                      = %d\n", shells[iii].ID);
//			TRACE("        centerID                = %d\n", shells[iii].centerID);
//			TRACE("        basisFunctions -> count = %d\n", shells[iii].basisFunctions.size());
//
//			for (unsigned int jjj = 0; jjj < shells[iii].basisFunctions.size(); ++jjj)
//			{
//				TRACE("            ContractedGaussianOrbital (BasisFunction) #%d\n", jjj);
//				TRACE("                ID                         = %d\n", shells[iii].basisFunctions[jjj].ID);
//				TRACE("                centerID                   = %d\n", shells[iii].basisFunctions[jjj].centerID);
//				TRACE("                shellID                    = %d\n", shells[iii].basisFunctions[jjj].shellID);
//				TRACE("                center                     = <%f, %f, %f>\n", shells[iii].basisFunctions[jjj].center.X, shells[iii].basisFunctions[jjj].center.Y, shells[iii].basisFunctions[jjj].center.Z);
//				TRACE("                angular momentum (n, l, m) = (%d, %d, %d)\n", shells[iii].basisFunctions[jjj].angularMomentum.n, shells[iii].basisFunctions[jjj].angularMomentum.l, shells[iii].basisFunctions[jjj].angularMomentum.m);
//				TRACE("                gaussian orbitals -> count = %d\n", shells[iii].basisFunctions[jjj].gaussianOrbitals.size());
//
//				for (unsigned int kkk = 0; kkk < shells[iii].basisFunctions[jjj].gaussianOrbitals.size(); ++kkk)
//				{
//					TRACE("                    GaussianOrbital #%d\n", kkk);
//					TRACE("                        alpha                      = %f\n", shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].alpha);
//					TRACE("                        coefficient                = %f\n", shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].coefficient);
//					TRACE("                        normalizationFactor        = %f\n", shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].normalizationFactor);
//					TRACE("                        ID                         = %d\n", shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].ID);
//					TRACE("                        centerID                   = %d\n", shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].centerID);
//					TRACE("                        shellID                    = %d\n", shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].shellID);
//					TRACE("                        center                     = <%f, %f, %f>\n", shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].center.X, shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].center.Y, shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].center.Z);
//					TRACE("                        angular momentum (n, l, m) = (%d, %d, %d)\n", shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].angularMomentum.n, shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].angularMomentum.l, shells[iii].basisFunctions[jjj].gaussianOrbitals[kkk].angularMomentum.m);
//
//				}
//			}
//		}
//
//	}

	AtomWithShells(unsigned int nrZ = 0, unsigned int nrElectrons = -1) : Atom(nrZ, nrElectrons) {}

//	void AddShell(const std::string& name)
//	{
//		Orbitals::ContractedGaussianShell shell;
//
//		for (auto c : name)	shell.AddOrbitals(c);
//
//		shells.emplace_back(std::move(shell));
//	}
//
//	void SetCenterForShells()
//	{
//		for (auto& shell : shells) shell.SetCenters(position);
//	}
//
//	void SetPosition(const Vector3D<double>& pos)
//	{
//		position = pos;
//		SetCenterForShells();
//	}
//
//	unsigned int CountNumberOfContractedGaussians() const;
//	unsigned int CountNumberOfGaussians() const;
//
//	void GetMaxQN(double alpha, Orbitals::QuantumNumbers::QuantumNumbers& maxQN) const
//	{
//		maxQN.l = maxQN.m = maxQN.n = 0;
//
//		for (const auto& shell : shells)
//			for (const auto& orbital : shell.basisFunctions)
//				for (const auto& gaussian : orbital.gaussianOrbitals)
//					if (alpha == gaussian.alpha)
//					{
//						maxQN.l = max(maxQN.l, orbital.angularMomentum.l);
//						maxQN.m = max(maxQN.m, orbital.angularMomentum.m);
//						maxQN.n = max(maxQN.m, orbital.angularMomentum.n);
//					}
//	}
//
//	unsigned int GetMaxAngularMomentum() const
//	{
//		unsigned int L = 0;
//
//		for (const auto& shell : shells)
//			for (const auto& orbital : shell.basisFunctions)
//				L = max(L, orbital.angularMomentum);
//
//		return L;
//	}
//
//	unsigned int GetMaxAngularMomentum(double alpha) const
//	{
//		unsigned int L = 0;
//
//		for (const auto& shell : shells)
//			for (const auto& orbital : shell.basisFunctions)
//				for (const auto& gaussian : orbital.gaussianOrbitals)
//					if (alpha == gaussian.alpha)
//						L = max(L, orbital.angularMomentum);
//
//		return L;
//	}
//
//	void Normalize()
//	{
//		for (auto& shell : shells) shell.Normalize();
//	}
};
}