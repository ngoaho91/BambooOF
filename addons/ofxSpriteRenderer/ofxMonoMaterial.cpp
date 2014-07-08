#include "ofxMonoMaterial.h"
#include "ofxSpriteRenderer.h"

#define DEBUG
ofxMonoMaterial::ofxMonoMaterial()
{
	glGenTextures(1, &m_TextureId);
	m_TextureSize[0] = m_TextureSize[1] = 0;
	m_ShaderProgramId = 0;
	m_ShaderLocationXYZ = -1;
	m_ShaderLocationUV = -1;
	m_ShaderLocationProjection = -1;
	m_ShaderLocationModelView = -1;
	m_ShaderLocationTransform = -1;
	m_ShaderLocationInvModelView = -1;
	m_ReferenceCount = 0;
}
ofxMonoMaterial::~ofxMonoMaterial()
{
	// TODO: do some glDeleteWhatever here
	//glDetachShader(program, id);
	//glDeleteShader(id);
	//FreeImage_Unload(m_ImageData);
}
void ofxMonoMaterial::LoadTexturePNG(const char* texture_file)
{
	m_ImageData = FreeImage_Load(FIF_PNG, texture_file, PNG_DEFAULT);
	FreeImage_FlipVertical(m_ImageData);
	unsigned int bpp = FreeImage_GetBPP(m_ImageData);
	unsigned int width = FreeImage_GetWidth(m_ImageData);
	unsigned int height = FreeImage_GetHeight(m_ImageData);
	BYTE* pixel_data = FreeImage_GetBits(m_ImageData);
	{
		GLenum format = bpp==24?GL_RGB:GL_RGBA;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_TextureId);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixel_data);
		m_TextureSize.x = width;
		m_TextureSize.y = height;
	}
	{
		GLint param = GL_CLAMP_TO_EDGE;
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,param);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,param);
	}
	{
		GLint param = GL_CLAMP_TO_EDGE;
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,param);
		if(param == GL_LINEAR_MIPMAP_LINEAR || GL_LINEAR_MIPMAP_NEAREST)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
			param = GL_LINEAR;
		}
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,param);
	}
	/*delete pixel_data;
	delete image_data;*/
}
bool ofxMonoMaterial::LoadShader(const char* vs_file, const char* fs_file)
{
	// generate id
	GLuint vs_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);
	m_ShaderProgramId = glCreateProgram();
	// load data
	ofBuffer vs_buffer = ofBufferFromFile(vs_file);
	string vs_string = vs_buffer.getText();
	const char* vs_source = vs_string.c_str();
	int vs_size = vs_buffer.getText().size()+1;
	glShaderSource(vs_id, 1, &vs_source, &vs_size);
	ofBuffer fs_buffer = ofBufferFromFile(fs_file);
	string fs_string = fs_buffer.getText();
	const char* fs_source = fs_string.c_str();
	int fs_size = fs_buffer.getText().size()+1;
	glShaderSource(fs_id, 1, &fs_source, &fs_size);
	// compile shader
	glCompileShader(vs_id);
	glCompileShader(fs_id);
	GLint vs_compile_status;
	glGetShaderiv(vs_id, GL_COMPILE_STATUS, &vs_compile_status);
	GLint fs_compile_status;
	glGetShaderiv(fs_id, GL_COMPILE_STATUS, &fs_compile_status);
	if(!(vs_compile_status && fs_compile_status))
	{
#ifdef _DEBUG
		GLint len = 0;
		glGetShaderiv(vs_id, GL_INFO_LOG_LENGTH, &len);
		if(len > 1)
		{
			char* info = new char[len];
			glGetShaderInfoLog(vs_id, len, NULL, info);
			printf("Error compiling vs shader:\n%s\n", info);
			delete[] info;
		}
		glGetShaderiv(fs_id, GL_INFO_LOG_LENGTH, &len);
		if(len > 1)
		{
			char* info = new char[len];
			glGetShaderInfoLog(fs_id, len, NULL, info);
			printf("Error compiling fs shader:\n%s\n", info);
			delete[] info;
		}
#endif
		glDeleteShader(vs_id);
		glDeleteShader(fs_id);
		return false;
	}
	// link program
	glAttachShader(m_ShaderProgramId, vs_id);
	glAttachShader(m_ShaderProgramId, fs_id);
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
			printf("Error linking program:\n%s\n", info);
			delete[] info;
		}
#endif
		glDeleteProgram(m_ShaderProgramId);
		return false;
	}
	return true;
}
void ofxMonoMaterial::BuildMaterial()
{
	m_ShaderLocationXYZ = glGetAttribLocation(m_ShaderProgramId, "a_position");
	m_ShaderLocationUV = glGetAttribLocation(m_ShaderProgramId, "a_uv");
	m_ShaderLocationProjection = glGetUniformLocation(m_ShaderProgramId, "u_projection_matrix");
	m_ShaderLocationModelView = glGetUniformLocation(m_ShaderProgramId, "u_modelview_matrix");
	m_ShaderLocationTransform = glGetUniformLocation(m_ShaderProgramId, "u_transform_matrix");
	m_ShaderLocationInvModelView = glGetUniformLocation(m_ShaderProgramId, "u_cam_inverse_matrix");
}
void ofxMonoMaterial::Bind()
{
	glUseProgram(m_ShaderProgramId);
	// vertices
	glEnableVertexAttribArray(m_ShaderLocationXYZ);
	glVertexAttribPointer(m_ShaderLocationXYZ, 3, GL_FLOAT, GL_FALSE, sizeof(ofxVertex), 0);
	// tex coords
	glEnableVertexAttribArray(m_ShaderLocationUV);
	glVertexAttribPointer(m_ShaderLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ofxVertex), (GLvoid*) offsetof( ofxVertex, UV[0]));
	// matrix
	//glUniformMatrix4fv(m_ShaderLocationProjection, 1, GL_FALSE, ofxRENDERER->GetProjectionMatrix().getPtr());
	//glUniformMatrix4fv(m_ShaderLocationModelView, 1, GL_FALSE, ofxRENDERER->GetModelViewMatrix().getPtr());
	glUniformMatrix4fv(m_ShaderLocationTransform, 1, GL_FALSE, ofxRENDERER->GetTransformation().getPtr());
	glUniformMatrix4fv(m_ShaderLocationInvModelView, 1, GL_FALSE, ofxRENDERER->GetInverseModelViewMatrix().getPtr());
	// shader textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
}
void ofxMonoMaterial::Unbind()
{
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}