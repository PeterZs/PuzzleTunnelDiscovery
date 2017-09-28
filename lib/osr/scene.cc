#include "scene.h"
#include <fstream>

namespace osr {
Scene::Scene()
	:xform_(xform_data_)
{
	// do nothing
}

Scene::Scene(std::shared_ptr<Scene> other)
	:xform_(other->xform_data_), shared_from_(other)
{
	// std::cerr<< "COPY Scene FROM " << other.get() << " TO " << this << std::endl;
	root_ = other->root_;
	bbox_ = other->bbox_;

	for (auto mesh : other->meshes_) {
		/*
		 * Copy Mesh object to get a different VAO for current OpenGL
		 * context.
		 */
		meshes_.emplace_back(new Mesh(mesh));
	}
	// std::cerr << "xform_data_: " << shared_from_->xform_data_[0][0] << std::endl;
	// std::cerr << "xform_: " << xform_[0][0] << std::endl;
}

Scene::~Scene()
{
	root_.reset();
	meshes_.clear();
}

void Scene::load(std::string filename, const glm::vec3* model_color)
{
	assert(std::ifstream(filename.c_str()).good());
	clear();

	using namespace Assimp;
	Assimp::Importer importer;
	uint32_t flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
			 aiProcess_FlipUVs | aiProcess_PreTransformVertices;
	const aiScene* scene = importer.ReadFile(filename, flags);

	const static std::vector<glm::vec3> meshColors = {
	    glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0),
	    glm::vec3(0.0, 0.0, 1.0), glm::vec3(1.0, 1.0, 0.0),
	    glm::vec3(1.0, 0.0, 1.0), glm::vec3(0.0, 1.0, 1.0),
	    glm::vec3(0.2, 0.3, 0.6), glm::vec3(0.6, 0.0, 0.8),
	    glm::vec3(0.8, 0.5, 0.2), glm::vec3(0.1, 0.4, 0.7),
	    glm::vec3(0.0, 0.7, 0.2), glm::vec3(1.0, 0.5, 1.0)};

	// generate all meshes
	for (size_t i = 0; i < scene->mNumMeshes; i++) {
		glm::vec3 color;
		if (model_color)
			color = *model_color;
		else
			color = meshColors[i % meshColors.size()];
		meshes_.emplace_back(new Mesh(scene->mMeshes[i], color));
	}

	// construct scene graph
	root_.reset(new Node(scene->mRootNode));

	center_ = glm::vec3(0.0f);
	vertex_total_number_ = 0;
	updateBoundingBox(root_.get(), glm::mat4());
	center_ = center_ / vertex_total_number_;
}

void Scene::updateBoundingBox(Node* node, glm::mat4 m)
{
	glm::mat4 xform = m * node->xform;
	for (auto i : node->meshes) {
		auto mesh = meshes_[i];
		for (const auto& vec : mesh->getVertices()) {
			glm::vec3 v =
			    glm::vec3(xform * glm::vec4(vec.position, 1.0));
			bbox_ << v;
			vertex_total_number_++;
			center_ += v;
		}
	}
	for (auto child : node->nodes) {
		updateBoundingBox(child.get(), xform);
	}
}

void Scene::render(GLuint program, Camera& camera, glm::mat4 m)
{
	// render(program, camera, m * xform, root);
	for (auto mesh : meshes_) {
		mesh->render(program, camera, m * xform_);
	}
}

void Scene::render(GLuint program, Camera& camera, glm::mat4 m, Node* node)
{
	glm::mat4 xform = m * node->xform;
#if 0
    if (node->meshes.size() > 0)
        std::cout << "matrix: " << std::endl << glm::to_string(xform) << std::endl;
#endif
	for (auto i : node->meshes) {
		auto mesh = meshes_[i];
		mesh->render(program, camera, xform);
	}
	for (auto child : node->nodes) {
		render(program, camera, xform, child.get());
	}
}

void Scene::clear()
{
	xform_ = glm::mat4();
	center_ = glm::vec3();
	bbox_ = BoundingBox();
	root_.reset();
	meshes_.clear();
}
}