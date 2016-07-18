#include "readtet.h"
#include <unistd.h>
#include <string>
#include <Eigen/Core>
#include <iostream>
#include <igl/barycenter.h>
#include <igl/viewer/Viewer.h>
#include <igl/jet.h>

using std::string;
using std::endl;
using std::cerr;
using std::fixed;
using std::vector;

void usage()
{
	std::cerr << "Options: -i <tetgen file prefix> -f <heat field data file>" << endl;
}

class KeyDown {
private:
	Eigen::MatrixXd& V_;
	Eigen::MatrixXi& E_;
	Eigen::MatrixXi& P_;
	Eigen::MatrixXd B;
	vector<Eigen::VectorXd>& fields_;
	int frameid_ = 0;

	void calibre_frameid()
	{
		frameid_ = std::max(frameid_, 0);
		frameid_ = std::min(int(fields_.size() - 1), frameid_);
	}

	vector<int> tetleft_;
	Eigen::MatrixXd V_temp_;
	Eigen::MatrixXi F_temp_;
	Eigen::VectorXd Z_temp_;
public:
	KeyDown(
		Eigen::MatrixXd& V,
		Eigen::MatrixXi& E,
		Eigen::MatrixXi& P,
		vector<Eigen::VectorXd>& fields
		)
		: V_(V), E_(E), P_(P), fields_(fields)
	{
		igl::barycenter(V,P,B);
		frameid_ = 0;
		std::cerr << "KeyDown constructor was called " << endl;
		adjust_slice_plane(0.5);
	}

	void adjust_slice_plane(double t)
	{
		Eigen::VectorXd v = B.col(2).array() - B.col(2).minCoeff();
		v /= v.col(0).maxCoeff();

		tetleft_.clear();
		for (unsigned i = 0; i < v.size(); ++i)
			if (v(i) < t)
				tetleft_.emplace_back(i);

		V_temp_.resize(tetleft_.size()*4,3);
		F_temp_.resize(tetleft_.size()*4,3);
		Z_temp_.resize(tetleft_.size()*4);
		for (unsigned i = 0; i < tetleft_.size(); ++i) {
			V_temp_.row(i*4+0) = V_.row(P_(tetleft_[i],0));
			V_temp_.row(i*4+1) = V_.row(P_(tetleft_[i],1));
			V_temp_.row(i*4+2) = V_.row(P_(tetleft_[i],2));
			V_temp_.row(i*4+3) = V_.row(P_(tetleft_[i],3));
			F_temp_.row(i*4+0) << (i*4)+0, (i*4)+1, (i*4)+3;
			F_temp_.row(i*4+1) << (i*4)+0, (i*4)+2, (i*4)+1;
			F_temp_.row(i*4+2) << (i*4)+3, (i*4)+2, (i*4)+0;
			F_temp_.row(i*4+3) << (i*4)+1, (i*4)+2, (i*4)+3;
		}
	}

	void update_frame(igl::viewer::Viewer& viewer)
	{
		Eigen::VectorXd& FV(fields_[frameid_]);
		for (unsigned i = 0; i < tetleft_.size(); ++i) {
#if 0
			Z_temp_(i*4+0) = FV(P_(tetleft_[i],0));
			Z_temp_(i*4+1) = FV(P_(tetleft_[i],1));
			Z_temp_(i*4+2) = FV(P_(tetleft_[i],2));
			Z_temp_(i*4+3) = FV(P_(tetleft_[i],3));
#else
			Z_temp_(i*4+0) = V_(P_(tetleft_[i],0), 2);
			Z_temp_(i*4+1) = V_(P_(tetleft_[i],1), 2);
			Z_temp_(i*4+2) = V_(P_(tetleft_[i],2), 2);
			Z_temp_(i*4+3) = V_(P_(tetleft_[i],3), 2);
#endif
		}
		Eigen::MatrixXd C(tetleft_.size()*4, 3);
		igl::jet(Z_temp_, true, C);

		viewer.data.clear();
		viewer.data.set_mesh(V_temp_, F_temp_);
		viewer.data.set_colors(C);
		viewer.data.set_face_based(false);
	}

	bool operator()(igl::viewer::Viewer& viewer, unsigned char key, int modifier)
	{
		using namespace std;
		using namespace Eigen;

		if (key == 'K') {
			frameid_ -= fields_.size()/10;
		} else if (key == 'J') {
			frameid_ += fields_.size()/10;
		}
		calibre_frameid();

		std::cerr << "Frame ID: " << frameid_
			<< "\tStepping: " << fields_.size() / 10
			<< "\tKey: " << key << " was pressed "
			<< endl;

		if (key >= '1' && key <= '9') {
			double t = double((key - '1')+1) / 9.0;
			adjust_slice_plane(t);
			update_frame(viewer);
			std::cerr << "Tet left: " << tetleft_.size() << endl;
		}

		return false;
	}

	void next_frame() 
	{
		frameid_++;
		std::cerr << frameid_ << ' ';
		calibre_frameid();
	}
};

void skip_to_needle(std::istream& fin, const string& needle)
{
	string s;
	do {
		fin >> s;
	} while(!fin.eof() && s != needle);
}

int main(int argc, char* argv[])
{
	int opt;
	string iprefix, ffn;
	while ((opt = getopt(argc, argv, "i:f:")) != -1) {
		switch (opt) {
			case 'i': 
				iprefix = optarg;
				break;
			case 'f':
				ffn = optarg;
				break;
			default:
				std::cerr << "Unrecognized option: " << optarg << endl;
				usage();
				return -1;
		}
	}
	if (iprefix.empty() || ffn.empty()) {
		std::cerr << "Missing input file" << endl;
		usage();
		return -1;
	}

	Eigen::MatrixXd V;
	Eigen::MatrixXi E;
	Eigen::MatrixXi P;
	vector<Eigen::VectorXd> fields;
	vector<double> times;
	try {
		readtet(V, E, P, iprefix);

		std::ifstream fin(ffn);
		if (!fin.is_open())
			throw std::runtime_error("Cannot open " + ffn + " for read");
		while (true) {
			skip_to_needle(fin, "t:");
			if (fin.eof())
				break;
			double t;
			size_t nvert;
			fin >> t >> nvert;
			times.emplace_back(t);
			Eigen::VectorXd field;
			field.resize(nvert);
			for(size_t i = 0; i < nvert; i++) {
				fin >> field(i);
			}
			fields.emplace_back(field);
		}
	} catch (std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}

	igl::viewer::Viewer viewer;
	KeyDown kd(V,E,P, fields);
	viewer.callback_key_pressed = [&kd](igl::viewer::Viewer& viewer, unsigned char key, int modifier) -> bool { return kd.operator()(viewer, key, modifier); } ;
	viewer.callback_pre_draw = [&kd](igl::viewer::Viewer& viewer) -> bool { kd.next_frame(); kd.update_frame(viewer); return false; };
	viewer.core.is_animating = true;
	viewer.core.animation_max_fps = 30.;
	viewer.launch();

	return 0;
}