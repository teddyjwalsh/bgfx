#pragma once

#include <tuple>
#include <regex>

#include "shader.h"
#include "texture.h"
#include "material_node.h"

namespace bgfx
{

extern std::string default_mat_vertex_shader;
extern std::string default_mat_fragment_shader;

class Material
{
public:

    typedef Node<std::string> MatNode;

private:

	
	ShaderProgram _program;
	std::shared_ptr<Shader> _vertex_shader;
	std::shared_ptr<Shader> _fragment_shader;
	std::vector<std::tuple<std::shared_ptr<Texture>, bool>> _textures;
	std::string _vertex_shader_text;
	std::string _fragment_shader_text;
	std::string _texture_sampler_text;
	std::string _texture_execution_text;
    std::vector<std::pair<std::string,std::string>> _uniforms;
    MatNode* _uv_node;
    MatNode* _pos_node;
    MatNode* _norm_node;
    MatNode* _frag_color_node;

    NodeGraph<std::string> _node_graph;

public:

	Material() :
		_vertex_shader_text(default_mat_vertex_shader),
		_fragment_shader_text(default_mat_fragment_shader),
		_texture_sampler_text(""),
		_texture_execution_text("")
	{
        _uv_node = new MatNode("uv");
        _uv_node->add_output("uv", "vec2");
        _pos_node = new MatNode("pos");
        _pos_node->add_output("pos", "vec3");
        _norm_node = new MatNode("norm");
        _norm_node->add_output("norm", "vec3");
        _node_graph.add_node(_uv_node);
        _node_graph.add_node(_pos_node);
        _node_graph.add_node(_norm_node);
        _frag_color_node = new MatNode("frag_color");
        _frag_color_node->add_input("gl_FragColor", "");
        _node_graph.add_node(_frag_color_node);
	}

	void use()
	{
		_program.use();
	}

	void set_transform(const glm::mat4& transform, std::string mat_name)
	{
		auto matrix_id = _program.get_uniform_location(mat_name);
		_program.set_uniform_mat4(matrix_id, transform);
	}

	MatNode* add_texture(std::shared_ptr<Texture> in_tex)
	{
        MatNode* tex_node = new MatNode(in_tex->name());
        tex_node->add_input("tex_coord", "vec2");
        tex_node->add_output("tex_value", "vec4");
        _node_graph.add_node(tex_node);
		_textures.push_back(std::make_tuple(in_tex, true));
        _uniforms.push_back(std::pair<std::string,std::string>("sampler2D", in_tex->name()));
        tex_node->data = "vec4 tex_value = texture(" + in_tex->name() + ", " + "tex_coord" + ");\n";
        return tex_node;
	}

	void compile()
	{
		// Compile our structures into our glsl fragment shader
        std::string uniform_shader_string;
        std::string node_assembly_string;
        for (auto& [uni_type, uni_name] : _uniforms)
        {
            uniform_shader_string += "uniform " + uni_type + " " + uni_name + ";\n";
        }
        auto sorted_nodes = _node_graph.sort_nodes();
        for (auto& node : sorted_nodes)
        {
            for (auto& [input_name, input] : node->inputs)
            {
                if (input.connection)
                {
                    node_assembly_string += input.type + " " + input.name + " = " + input.connection->name + ";\n"; 
                }
            }
            node_assembly_string += node->data; 
            printf("Node name: %s\n", node->data.c_str());
        }
		_fragment_shader_text = std::regex_replace(_fragment_shader_text, 
			std::regex("//_UNIFORMS_"), 
			uniform_shader_string);
		_fragment_shader_text = std::regex_replace(_fragment_shader_text,
			std::regex("//_NODE_ASSEMBLY_"),
			node_assembly_string);
		printf("complete frag text: %s\n", _fragment_shader_text.c_str());
		_vertex_shader = std::make_shared<Shader>(Shader::Type::Vertex, _vertex_shader_text, true);
		_fragment_shader = std::make_shared<Shader>(Shader::Type::Fragment, _fragment_shader_text, true);
		_program.attach_shader(_vertex_shader);
		_program.attach_shader(_fragment_shader);
		_program.link();
	}

	void make_connection(
		std::shared_ptr<NodeOutput> output,
		std::shared_ptr<NodeInput> input)
	{

	}

	MatNode* uv_node()
	{
		return _uv_node;
	}
	MatNode* pos_node()
	{
		return _pos_node;
	}
	MatNode* normal_node()
	{
		return _norm_node;
	}
	MatNode* frag_color_node()
	{
		return _frag_color_node;
	}
};

}  // namespace bgfx
