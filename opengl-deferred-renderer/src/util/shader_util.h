#ifndef SHADER_UTIL_H
#define SHADER_UTIL_H

#include <glm/glm.hpp>
#include <vector>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define MAX_BONE_INFLUENCE 4

std::string get_file_contents(std::string filename);
unsigned int compile_shader(const std::string& shaderSource, unsigned int shaderType);
unsigned int texture_from_file(const char* _filename, const std::string& directory, bool gamma = false);
unsigned int load_cubemap(std::vector<std::string> faces);

class Shader
{
    public:
        // the program ID
        unsigned int ID;

        // constructor reads and builds the shader
        Shader(const std::string& vertexPath, const std::string& fragmentPath);
        Shader(const std::string& vertexPath, const std::string& fragmentPath, const std::string& geometryPath);
        // use/activate the shader
        void use();
        // utility uniform functions
        void setBool(const std::string& name, bool value) const;
        void setInt(const std::string& name, int value) const;
        void setFloat(const std::string& name, float value) const;
        void setVec2f(const std::string& name, float x, float y) const;
        void setVec2f(const std::string& name, glm::vec2 vector) const;
        void setVec3f(const std::string& name, float x, float y, float z) const;
        void setVec3f(const std::string& name, glm::vec3 vector) const;
        void setVec4f(const std::string& name, float x, float y, float z, float w) const;
        void setVec4f(const std::string& name, glm::vec4 vector) const;
        void setMat3(const std::string& name, glm::mat3 matrix) const;
        void setMat4(const std::string& name, glm::mat4 matrix) const;
};

class Material {
    public:
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shine;
        Material(glm::vec3 _diffuse, glm::vec3 _specular, float _shine);
};

class Light {
    public:
        glm::vec3 position;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        Light(glm::vec3 _position, glm::vec3 _ambient, glm::vec3 _diffuse, glm::vec3 _specular);
        Light(glm::vec3 _position, glm::vec3 _lightColor);
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    // tangent
    glm::vec3 tangent;
    // bitangent
    glm::vec3 bitangent;
    //bone indexes which will influence this vertex
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    //weights from each bone
    float m_Weights[MAX_BONE_INFLUENCE];
};
struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
    public:
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
        void draw(Shader& shader);
        unsigned int VAO;
    private:
        unsigned int VBO, EBO;
        void _setupMesh();
};

class Model {
    public:
        Model(std::string path);
        void draw(Shader& shader);
        std::vector<Mesh> meshes;
    private:
        std::string _directory;

        void _loadModel(std::string path);
        void _processNode(aiNode* node, const aiScene* scene);
        Mesh _processMesh(aiMesh* mesh, const aiScene* scene);
        std::vector<Texture> _loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

};

#endif