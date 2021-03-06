#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include <Eigen/Core>

#include "waveblocks/types.hpp"
#include "waveblocks/wavepackets/hawp_commons.hpp"
#include "waveblocks/wavepackets/hawp_paramset.hpp"
#include "waveblocks/wavepackets/shapes/tiny_multi_index.hpp"
#include "waveblocks/wavepackets/shapes/shape_enumerator.hpp"
#include "waveblocks/wavepackets/shapes/shape_hypercubic.hpp"
#include "waveblocks/innerproducts/gauss_hermite_qr.hpp"
#include "waveblocks/innerproducts/tensor_product_qr.hpp"
#include "waveblocks/innerproducts/genz_keister_qr.hpp"
#include "waveblocks/innerproducts/homogeneous_inner_product.hpp"
#include "waveblocks/innerproducts/inhomogeneous_inner_product.hpp"
#include "waveblocks/innerproducts/vector_inner_product.hpp"
#include "waveblocks/utilities/stdarray2stream.hpp"


using namespace waveblocks;

void test1DGaussHermite()
{
    std::cout << "\n1D Homogeneous Gauss-Hermite Test\n";
    std::cout <<   "---------------------------------\n";

    const real_t eps = 0.2;
    const dim_t D = 1;
    const dim_t n_coeffs = 10;
    const dim_t order = 8;
    using MultiIndex = wavepackets::shapes::TinyMultiIndex<unsigned short, D>;
    using QR = innerproducts::GaussHermiteQR<order>;

    // Set up sample 1D wavepacket.
    wavepackets::shapes::ShapeEnumerator<D, MultiIndex> enumerator;
    wavepackets::shapes::ShapeEnum<D, MultiIndex> shape_enum = enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(n_coeffs));
    wavepackets::HaWpParamSet<D> param_set;
    std::cout << param_set << std::endl;
    Coefficients coeffs = Coefficients::Ones(n_coeffs,1);

    // Print QR nodes and weights.
    //std::cout << "nodes: {";
    //for (auto x : QR::nodes()) std::cout << " " << x;
    //std::cout << " }\n";

    //std::cout << "weights: {";
    //for (auto x : QR::weights()) std::cout << " " << x;
    //std::cout << " }\n";

    wavepackets::ScalarHaWp<D, MultiIndex> packet;
    packet.eps() = eps;
    packet.parameters() = param_set;
    packet.shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(shape_enum);
    packet.coefficients() = coeffs;

    // Calculate inner product matrix, print it.
    using IP = innerproducts::HomogeneousInnerProduct<D, MultiIndex, QR>;
    CMatrix<Eigen::Dynamic, Eigen::Dynamic> mat = IP::build_matrix(packet);

    //std::cout << "IP matrix:\n" << mat << std::endl;

    // Calculate quadrature.
    std::cout << "Quadrature: " << IP::quadrature(packet) << "\n";
}

void test1DGaussHermiteOperator()
{
    std::cout << "\n1D Homogeneous Gauss-Hermite Custom Operator Test\n";
    std::cout <<   "-------------------------------------------------\n";

    const real_t eps = 0.2;
    const dim_t D = 1;
    const dim_t n_coeffs = 10;
    const dim_t order = 8;
    using MultiIndex = wavepackets::shapes::TinyMultiIndex<unsigned short, D>;
    using QR = innerproducts::GaussHermiteQR<order>;
    using CMatrix1X = CMatrix<1, Eigen::Dynamic>;
    using RMatrixD1 = RMatrix<D, 1>;
    using CMatrixDX = CMatrix<D, Eigen::Dynamic>;

    // Set up sample 1D wavepacket.
    wavepackets::shapes::ShapeEnumerator<D, MultiIndex> enumerator;
    wavepackets::shapes::ShapeEnum<D, MultiIndex> shape_enum = enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(n_coeffs));
    wavepackets::HaWpParamSet<D> param_set;
    std::cout << param_set << std::endl;
    Coefficients coeffs = Coefficients::Ones(n_coeffs, 1);

    // Print QR nodes and weights.
    //std::cout << "nodes: {";
    //for (auto x : QR::nodes()) std::cout << " " << x;
    //std::cout << " }\n";

    //std::cout << "weights: {";
    //for (auto x : QR::weights()) std::cout << " " << x;
    //std::cout << " }\n";

    wavepackets::ScalarHaWp<D, MultiIndex> packet;
    packet.eps() = eps;
    packet.parameters() = param_set;
    packet.shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(shape_enum);
    packet.coefficients() = coeffs;

    // Calculate inner product matrix, print it.
    // Use an operator that returns a sequence 1, 2, ..., number_nodes.
    using IP = innerproducts::HomogeneousInnerProduct<D, MultiIndex, QR>;
    auto op =
        [] (CMatrixDX nodes, RMatrixD1 pos) -> CMatrix1X
    {
        (void)pos;
        const dim_t n_nodes = nodes.cols();
        CMatrix1X result(1, n_nodes);
        for(int i = 0; i < n_nodes; ++i) result(0, i) = i+1;
        return result;
    };
    CMatrix<Eigen::Dynamic, Eigen::Dynamic> mat = IP::build_matrix(packet, op);

    //std::cout << "IP matrix:\n" << mat << std::endl;

    // Calculate quadrature.
    std::cout << "Quadrature: " << IP::quadrature(packet, op) << "\n";
}

void test3DGaussHermite()
{
    std::cout << "\n3D Homogeneous Tensor-Product Gauss-Hermite Test\n";
    std::cout <<   "------------------------------------------------\n";

    const real_t eps = 0.2;
    const dim_t D = 3;
    const dim_t n_coeffs = 5;
    using MultiIndex = wavepackets::shapes::TinyMultiIndex<unsigned short, D>;

    // Set up sample 3D wavepacket.
    wavepackets::shapes::ShapeEnumerator<D, MultiIndex> enumerator;
    wavepackets::shapes::ShapeEnum<D, MultiIndex> shape_enum = enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(n_coeffs));
    wavepackets::HaWpParamSet<D> param_set;
    std::cout << param_set << std::endl;
    Coefficients coeffs = Coefficients::Ones(std::pow(n_coeffs, D),1);
    for(int i = 0; i < coeffs.size(); ++i) coeffs[i] = i+1;
    wavepackets::ScalarHaWp<D, MultiIndex> packet;
    packet.eps() = eps;
    packet.parameters() = param_set;
    packet.shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(shape_enum);
    packet.coefficients() = coeffs;
    //std::cout << "Coefficients:\n";
    //std::cout << coeffs << "\n";

    using TQR = innerproducts::TensorProductQR<innerproducts::GaussHermiteQR<3>,
                                               innerproducts::GaussHermiteQR<4>,
                                               innerproducts::GaussHermiteQR<5>>;
    //TQR::NodeMatrix nodes;
    //TQR::WeightVector weights;
    //std::tie(nodes, weights) = TQR::nodes_and_weights();
    //std::cout << "number of nodes: " << TQR::number_nodes() << "\n";
    //std::cout << "node matrix:\n" << nodes << "\n";
    //std::cout << "weight vector:\n" << weights << "\n";

    // Calculate inner product matrix, print it.
    using IP = innerproducts::HomogeneousInnerProduct<D, MultiIndex, TQR>;
    CMatrix<Eigen::Dynamic, Eigen::Dynamic> mat =
        IP::build_matrix(packet);

    //std::cout << "IP matrix diagonal:\n";
    //for(int i = 0; i < mat.rows(); ++i)
    //{
    //    std::cout << " " << mat(i, i);
    //    if(i%3 == 2) std::cout << "\n";
    //}
    //std::cout << "\n";

    // Calculate quadrature.
    std::cout << "Quadrature: " << IP::quadrature(packet) << "\n";

    TQR::clear_cache();
}

void test1DInhomog()
{
    std::cout << "\n1D Inhomogeneous Gauss-Hermite Test\n";
    std::cout <<   "-----------------------------------\n";

    const real_t eps = 0.2;
    const dim_t D = 1;
    const dim_t n_coeffs1 = 10, n_coeffs2 = 12;
    const dim_t order = 8;
    using MultiIndex = wavepackets::shapes::TinyMultiIndex<unsigned short, D>;
    using QR = innerproducts::GaussHermiteQR<order>;

    // Set up sample 1D wavepacket.
    wavepackets::shapes::ShapeEnumerator<D, MultiIndex> enumerator;
    wavepackets::shapes::ShapeEnum<D, MultiIndex> shape_enum1 = enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(n_coeffs1));
    wavepackets::shapes::ShapeEnum<D, MultiIndex> shape_enum2 = enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(n_coeffs2));
    wavepackets::HaWpParamSet<D> param_set1, param_set2;
    Coefficients coeffs1 = Coefficients::Ones(n_coeffs1,1);
    Coefficients coeffs2 = Coefficients::Constant(n_coeffs2,1, 1.4);

    wavepackets::ScalarHaWp<D, MultiIndex> packet1;
    packet1.eps() = eps;
    packet1.parameters() = param_set1;
    packet1.shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(shape_enum1);
    packet1.coefficients() = coeffs1;

    wavepackets::ScalarHaWp<D, MultiIndex> packet2;
    packet2.eps() = eps;
    packet2.parameters() = param_set2;
    packet2.shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(shape_enum2);
    packet2.coefficients() = coeffs2;

    // Calculate inner product matrix, print it.
    using IP = innerproducts::InhomogeneousInnerProduct<D, MultiIndex, QR>;
    CMatrix<Eigen::Dynamic, Eigen::Dynamic> mat = IP::build_matrix(packet1, packet2);

    //std::cout << "IP matrix:\n" << mat << std::endl;

    // Calculate quadrature.
    std::cout << "Quadrature: " << IP::quadrature(packet1, packet2) << "\n";
}

void testVector()
{
    std::cout << "\nMulti-component Test\n";
    std::cout <<   "--------------------\n";

    const real_t eps = 0.2;
    const dim_t D = 1;
    const dim_t order = 8;
    using MultiIndex = wavepackets::shapes::TinyMultiIndex<unsigned short, D>;
    using QR = innerproducts::GaussHermiteQR<order>;

    // Set up sample 1D wavepacket.
    wavepackets::shapes::ShapeEnumerator<D, MultiIndex> enumerator;
    wavepackets::HaWpParamSet<D> param_set1, param_set2;

    wavepackets::HomogeneousHaWp<D, MultiIndex> packet1(2);
    packet1.eps() = eps;
    packet1.parameters() = param_set1;
    packet1.component(0).shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(5)));
    packet1.component(0).coefficients() = Coefficients::Constant(5,1, 1.0);
    packet1.component(1).shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(6)));
    packet1.component(1).coefficients() = Coefficients::Constant(6,1, 1.5);

    wavepackets::HomogeneousHaWp<D, MultiIndex> packet2(3);
    packet2.eps() = eps;
    packet2.parameters() = param_set2;
    packet2.component(0).shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(3)));
    packet2.component(0).coefficients() = Coefficients::Constant(3,1, 1.4);
    packet2.component(1).shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(4)));
    packet2.component(1).coefficients() = Coefficients::Constant(4,1, 1.2);
    packet2.component(2).shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(7)));
    packet2.component(2).coefficients() = Coefficients::Constant(7,1, 1.0);

    // Calculate inner product matrix, print it.
    using IP = innerproducts::VectorInnerProduct<D, MultiIndex, QR>;
    CMatrix<Eigen::Dynamic, Eigen::Dynamic> mat = IP::build_matrix_inhomog(packet1, packet2);
    //IP::build_matrix(packet1);

    std::cout << "IP matrix:\n" << mat << std::endl;

    // Calculate quadrature.
    std::cout << "Quadrature: " << IP::quadrature_inhomog(packet1, packet2) << "\n";
    //std::cout << "Quadrature: " << IP::quadrature(packet1) << "\n";
}

void test3DGenzKeister()
{
    std::cout << "\n3D Homogeneous Genz-Keister Test\n";
    std::cout <<   "--------------------------------\n";

    const real_t eps = 0.2;
    const dim_t D = 3;
    const dim_t level = 4;
    const dim_t n_coeffs = 5;
    using MultiIndex = wavepackets::shapes::TinyMultiIndex<unsigned short, D>;

    // Set up sample 3D wavepacket.
    wavepackets::shapes::ShapeEnumerator<D, MultiIndex> enumerator;
    wavepackets::shapes::ShapeEnum<D, MultiIndex> shape_enum = enumerator.generate(wavepackets::shapes::HyperCubicShape<D>(n_coeffs));
    wavepackets::HaWpParamSet<D> param_set;
    std::cout << param_set << std::endl;
    Coefficients coeffs = Coefficients::Ones(std::pow(n_coeffs, D),1);
    //for(int i = 0; i < coeffs.size(); ++i) coeffs[i] = i+1;
    wavepackets::ScalarHaWp<D, MultiIndex> packet;
    packet.eps() = eps;
    packet.parameters() = param_set;
    packet.shape() = std::make_shared<wavepackets::shapes::ShapeEnum<D,MultiIndex>>(shape_enum);
    packet.coefficients() = coeffs;
    //std::cout << "Coefficients:\n";
    //std::cout << coeffs << "\n";

    using QR = innerproducts::GenzKeisterQR<D, level>;
    QR::NodeMatrix nodes;
    QR::WeightVector weights;
    std::tie(nodes, weights) = QR::nodes_and_weights();
    std::cout << "number of nodes: " << QR::number_nodes() << "\n";
    std::cout << "node matrix:\n" << nodes << "\n";
    std::cout << "weight vector:\n" << weights << "\n";

    // Calculate inner product matrix, print it.
    using IP = innerproducts::HomogeneousInnerProduct<D, MultiIndex, QR>;
    //CMatrix<Eigen::Dynamic, Eigen::Dynamic> mat =
    //    IP::build_matrix(packet);

    // Calculate quadrature.
    std::cout << "Quadrature: " << IP::quadrature(packet) << "\n";

    QR::clear_cache();
}

int main()
{
    //test1DGaussHermite();
    //test1DGaussHermiteOperator();
    //test3DGaussHermite();
    //test1DInhomog();
    //testVector();
    test3DGenzKeister();

    return 0;
}
