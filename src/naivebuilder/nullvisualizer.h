#ifndef NULL_VISUALIZER_H
#define NULL_VISUALIZER_H

#include <time.h>
#include <vector>
#include <Eigen/Core>

/*
 * NullVisualizer: concept of Visualizer used by GOctreePathBuilder
 */
class NullVisualizer {
public:
	static void initialize() { last_time_ = ::time(NULL); }
	template<typename Node>
	static void visAdj(Node*, Node* ) {}

	template<typename Node>
	static void visAggAdj(Node*, Node* ) {}
	template<typename Node>
	static void withdrawAggAdj(Node*) {}

	template<typename Node>
	static void visSplit(Node*) {}
	template<typename Node>
	static void visCertain(Node*) {}

	static void visAggPath(const std::vector<Eigen::VectorXd>&) {}

	template<typename Node>
	static void trackFurestCube(Node* cube, Node* init_cube) {}

	static bool timerAlarming() { return ::time(NULL) > last_time_; }
	static void periodicalReport() {}
	static void rearmTimer() { last_time_ = ::time(NULL); }
	static void pause() {}

	struct Attribute {
	};
protected:
	static time_t last_time_;
};


#endif
