#include "ofxShaderProgram.h"
#include "ofxShaderCache.h"
#include "ofxSpriteRenderer.h"

ofxShaderProgram::ofxShaderProgram()
{
	m_ShaderProgramId = glCreateProgram();
}
ofxShaderProgram::~ofxShaderProgram()
{
	glDetachShader(m_ShaderProgramId, m_VertexShader->GetShaderId());
	glDetachShader(m_ShaderProgramId, m_FragmentShader->GetShaderId());
	glDeleteProgram(m_ShaderProgramId);
}
bool ofxShaderProgram::Load(string path)
{
	return Load(path+".vert", path+".frag");
}
bool ofxShaderProgram::Load(string vs_path, string fs_path)
{
	m_VertexShader = ofxSHADERCACHE->GetResource(vs_path);
	m_FragmentShader = ofxSHADERCACHE->GetResource(fs_path);
	// link program
	glAttachShader(m_ShaderProgramId, m_VertexShader->GetShaderId());
	glAttachShader(m_ShaderProgramId, m_FragmentShader->GetShaderId());
	glLinkProgram(m_ShaderProgramId);
	GLint link_status;
	glGetProgramiv(m_ShaderProgramId, GL_LINK_STATUS, &link_status);
	if(!link_status)
	{
#ifdef _DEBUG
		GLint len = 0;
		glGetProgramiv(m_ShaderProgramId, GL_INFO_LOG_LENGTH, &len);
		if(len > 1)
		{
			char* info = new char[len];
			glGetProgramInfoLog(m_ShaderProgramId, len, NULL, info);
			printf("Error linking program[%s,%s]:\n%s\n", vs_path.c_str(), fs_path.c_str(), info);
			delete[] info;
		}
#endif
		glDeleteProgram(m_ShaderProgramId);
		return false;
	}
	{
		m_AttributeMap["a_position"]			= glGetAttribLocation(m_ShaderProgramId, "a_position");
		m_AttributeMap["a_uv"]					= glGetAttribLocation(m_ShaderProgramId, "a_uv");
		m_AttributeMap["a_opacity"]				= glGetAttribLocation(m_ShaderProgramId, "a_opacity");
		m_UniformMap["u_texture"]				= glGetUniformLocation(m_ShaderProgramId, "u_texture");
		m_UniformMap["u_modelview_matrix"]		= glGetUniformLocation(m_ShaderProgramId, "u_modelview_matrix");
		m_UniformMap["u_projection_matrix"]		= glGetUniformLocation(m_ShaderProgramId, "u_projection_matrix");
		m_UniformMap["u_transform_matrix"]		= glGetUniformLocation(m_ShaderProgramId, "u_transform_matrix");
		m_UniformMap["u_cam_inverse_matrix"]	= glGetUniformLocation(m_ShaderProgramId, "u_cam_inverse_matrix");
	}
	return true;
}
void ofxShaderProgram::Bind()
{
	glUseProgram(m_ShaderProgramId);
	{
		glEnableVertexAttribArray	(m_AttributeMap["a_position"]);
		glVertexAttribPointer		(m_AttributeMap["a_position"],			3, GL_FLOAT, GL_FALSE, 
																				sizeof(ofxVertex), (GLvoid*) offsetof( ofxVertex, x));
		glEnableVertexAttribArray	(m_AttributeMap["a_uv"]);
		glVertexAttribPointer		(m_AttributeMap["a_uv"],				2, GL_FLOAT, GL_FALSE, 
																				sizeof(ofxVertex), (GLvoid*) offsetof( ofxVertex, u));
		glEnableVertexAttribArray	(m_AttributeMap["a_opacity"]);
		glVertexAttribPointer		(m_AttributeMap["a_opacity"],			1, GL_FLOAT, GL_FALSE, 
																				sizeof(ofxVertex), (GLvoid*) offsetof( ofxVertex, opacity));
	}
	{
		glUniform1i					(m_UniformMap["u_texture"],				0);
		glUniformMatrix4fv			(m_UniformMap["u_modelview_matrix"],	1, GL_FALSE, ofxRENDERER->GetModelViewMatrix().getPtr());
		glUniformMatrix4fv			(m_UniformMap["u_projection_matrix"],	1, GL_FALSE, ofxRENDERER->GetProjectionMatrix().getPtr());
		glUniformMatrix4fv			(m_UniformMap["u_transform_matrix"],	1, GL_FALSE, ofxRENDERER->GetTransformation().getPtr());
		glUniformMatrix4fv			(m_UniformMap["u_cam_inverse_matrix"],	1, GL_FALSE, ofxRENDERER->GetInverseModelViewMatrix().getPtr());
	}
}
void ofxShaderProgram::Unbind()
{
	glUseProgram(0);
}
void ofxShaderProgram::IncreaseReference()
{
	ofxResource::IncreaseReference();
	m_VertexShader->IncreaseReference();
	m_FragmentShader->IncreaseReference();
}
void ofxShaderProgram::DecreaseReference()
{
	ofxResource::DecreaseReference();
	m_VertexShader->DecreaseReference();
	m_FragmentShader->DecreaseReference();
}
bool ofxShaderProgram::IsUnused()
{
	return ofxResource::IsUnused();
}