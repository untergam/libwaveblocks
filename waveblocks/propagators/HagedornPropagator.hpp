#pragma once

#include "waveblocks/propagators/Propagator.hpp"
#include "waveblocks/propagators/SplittingParameters.hpp"

namespace waveblocks {
namespace propagators {

namespace utils = utilities;

/** \file
 * \brief Implements the Hagedorn Propagator
 */
template <int N, int D, typename MultiIndex_t, typename MDQR_t, typename Potential_t, typename Packet_t>
class HagedornPropagator : public Propagator<HagedornPropagator<N,D,MultiIndex_t,MDQR_t,Potential_t,Packet_t>,N,D,MultiIndex_t,MDQR_t,Potential_t,Packet_t>, public SplittingParameters {

	public:

		using Propagator<HagedornPropagator<N,D,MultiIndex_t,MDQR_t,Potential_t,Packet_t>,N,D,MultiIndex_t,MDQR_t,Potential_t,Packet_t>::Propagator; // inherit constructor

		std::string getName() {
			return "Hagedorn";
		}

		void propagate(const real_t Dt) {

			// Hagedorn
			this->stepT(Dt/2);
			this->stepU(Dt);
			this->stepW(Dt);
			this->stepT(Dt/2);

			// Semiclassical
			// TODO: compute M
			// TODO: make sure that M is even, otherwise M/2 + M/2 != M
			// int M = 4;
			// assert(M%2==0);
			// this->intSplit(*this,Dt/2,M/2,coefKL10);
			// this->stepW(Dt);
			// this->intSplit(*this,Dt/2,M/2,coefKL10);


			// // Magnus
			// // TODO: solve problems with continuous square root
			// // TODO: introduce additional temporary variables A1_, A2_ for derived class
			// CMatrix<Eigen::Dynamic,Eigen::Dynamic> A1_, A2_;
			// // TODO: consider saving sqrt(3) as a private class constant
			// // TODO: auto& wpacket = this->wpacket_;
			// real_t h1 = (3-std::sqrt(3))/6;
			// real_t h2 = std::sqrt(3)/6;
			// int N1 = 2; // TODO: put right formula
			// int N2 = 2; // TODO: put right formula
			// this->intSplit(h1,N1); // TODO: uncomment
			// this->buildF(A1_);
			// A1_ *= complex_t(0,-Dt/(this->wpacket_.eps()*this->wpacket_.eps()));
			// this->intSplit(h2,N2); // TODO: uncomment
			// this->buildF(A2_);
			// A2_ *= complex_t(0,-Dt/(this->wpacket_.eps()*this->wpacket_.eps()));
			// this->F_ = .5*(A1_+A2_) + std::sqrt(3)/12*(A1_*A2_-A2_*A1_);
			// CVector<Eigen::Dynamic> coefs = utils::PacketToCoefficients<Packet_t>::to(this->wpacket_); // get coefficients from packet
			// coefs = (this->F_).exp() * coefs;
			// utils::PacketToCoefficients<Packet_t>::from(coefs,this->wpacket_); // update packet from coefficients
			// this->intSplit(h1,N1); // TODO: uncomment

			// // Pre764
			// // TODO: introduce additional coefficient vectors Z_, Y_, alpha_, beta_ for derived class
			// const int M = 1 + std::floor(std::sqrt(Dt*std::pow(this->wpacket_.eps(),-.75)));
			// // TODO: v, k could even be class constants v_, k_
			// const int v = 6;
			// const int k = 4;

			// std::vector<real_t> Z_(v), Y_(v); // TODO: populate with coefficients
			// std::vector<real_t> alpha_(k), beta_(k); // TODO: populate with coefficients

			// // PRE: TODO: move to separate function, not in propagate
			// for(unsigned j=0; j<v; ++j) {
			// 	this->intSplit(-Z_[j]*Dt,M);
			// 	this->stepW(-Y_[j]*Dt);
			// }
			// // PROPAGATE:
			// for(unsigned j=0; j<k; ++j) {
			// 	this->stepW(alpha_[j]*Dt);
			// 	this->intSplit(beta_[j]*Dt,M);
			// }
			// // POST: TODO: move to separate function, not in propagate
			// for(unsigned j=0; j<v; ++j) {
			// 	this->stepW(Y_[j]*Dt);
			// 	this->intSplit(Z_[j]*Dt,M);
			// }



			// // McL42
			// std::vector<real_t> a_(3), b_(2); // TODO: a_ b_ rename
			// // int M = 10; // TODO: compute M
			// this->intSplit(a_[0]*Dt,M);
			// this->stepW(b_[0]*Dt);
			// this->intSplit(a_[1]*Dt,M);
			// this->stepW(b_[1]*Dt);
			// this->intSplit(a_[2]*Dt,M);

		}

		void pre_propagate(const real_t) {}
		void post_propagate(const real_t) {}

};


} // namespace propagators
} // namespace waveblocks
