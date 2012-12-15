/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2011 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "CmGLGpuProgram.h"
#include "CmException.h"

using namespace CamelotEngine;

GLenum getGLShaderType(GpuProgramType programType)
{
	switch (programType)
	{
		case GPT_VERTEX_PROGRAM:
		default:
			return GL_VERTEX_PROGRAM_ARB;
		case GPT_GEOMETRY_PROGRAM:
			return GL_GEOMETRY_PROGRAM_NV;
		case GPT_FRAGMENT_PROGRAM:
			return GL_FRAGMENT_PROGRAM_ARB;
	}
}

GLGpuProgram::GLGpuProgram(const String& source, const String& entryPoint, const String& language, 
						   GpuProgramType gptype, GpuProgramProfile profile, bool isAdjacencyInfoRequired) 
    : GpuProgram(source, entryPoint, language, gptype, profile, isAdjacencyInfoRequired)
{
    //if (createParamDictionary("GLGpuProgram"))
    //{
    //    setupBaseParamDictionary();
    //}
}

GLGpuProgram::~GLGpuProgram()
{
    // have to call this here reather than in Resource destructor
    // since calling virtual methods in base destructors causes crash
    unload_internal(); 
}

GLuint GLGpuProgram::getAttributeIndex(VertexElementSemantic semantic, CamelotEngine::UINT32 index)
{
	return getFixedAttributeIndex(semantic, index);
}

GLuint GLGpuProgram::getFixedAttributeIndex(VertexElementSemantic semantic, CamelotEngine::UINT32 index)
{
	// Some drivers (e.g. OS X on nvidia) incorrectly determine the attribute binding automatically
	// and end up aliasing existing built-ins. So avoid! Fixed builtins are: 

	//  a  builtin				custom attrib name
	// ----------------------------------------------
	//	0  gl_Vertex			vertex
	//  1  n/a					blendWeights		
	//	2  gl_Normal			normal
	//	3  gl_Color				colour
	//	4  gl_SecondaryColor	secondary_colour
	//	5  gl_FogCoord			fog_coord
	//  7  n/a					blendIndices
	//	8  gl_MultiTexCoord0	uv0
	//	9  gl_MultiTexCoord1	uv1
	//	10 gl_MultiTexCoord2	uv2
	//	11 gl_MultiTexCoord3	uv3
	//	12 gl_MultiTexCoord4	uv4
	//	13 gl_MultiTexCoord5	uv5
	//	14 gl_MultiTexCoord6	uv6, tangent
	//	15 gl_MultiTexCoord7	uv7, binormal
	switch(semantic)
	{
	case VES_POSITION:
		return 0;
	case VES_BLEND_WEIGHTS:
		return 1;
	case VES_NORMAL:
		return 2;
	case VES_DIFFUSE:
		return 3;
	case VES_SPECULAR:
		return 4;
	case VES_BLEND_INDICES:
		return 7;
	case VES_TEXTURE_COORDINATES:
		return 8 + index;
	case VES_TANGENT:
		return 14;
	case VES_BITANGENT:
		return 15;
	default:
		assert(false && "Missing attribute!");
		return 0;
	};

}

bool GLGpuProgram::isAttributeValid(VertexElementSemantic semantic, CamelotEngine::UINT32 index)
{
	// default implementation
	switch(semantic)
	{
		case VES_POSITION:
		case VES_NORMAL:
		case VES_DIFFUSE:
		case VES_SPECULAR:
		case VES_TEXTURE_COORDINATES:
			return false;
		case VES_BLEND_WEIGHTS:
		case VES_BLEND_INDICES:
		case VES_BITANGENT:
		case VES_TANGENT:
			return true; // with default binding
	}

    return false;
}

GLArbGpuProgram::GLArbGpuProgram(const String& source, const String& entryPoint, const String& language, 
								 GpuProgramType gptype, GpuProgramProfile profile, bool isAdjacencyInfoRequired) 
	: GLGpuProgram(source, entryPoint, language, gptype, profile, isAdjacencyInfoRequired)
{
    glGenProgramsARB(1, &mProgramID);
	mProgramType = getGLShaderType(gptype);
}

GLArbGpuProgram::~GLArbGpuProgram()
{
    // have to call this here reather than in Resource destructor
    // since calling virtual methods in base destructors causes crash
    unload_internal(); 
}

void GLArbGpuProgram::bindProgram(void)
{
    glEnable(mProgramType);
    glBindProgramARB(mProgramType, mProgramID);
}

void GLArbGpuProgram::unbindProgram(void)
{
    glBindProgramARB(mProgramType, 0);
    glDisable(mProgramType);
}

void GLArbGpuProgram::bindProgramParameters(GpuProgramParametersSharedPtr params, CamelotEngine::UINT16 mask)
{
    GLenum type = getGLShaderType(mType);
    
	// only supports float constants
	GpuLogicalBufferStructPtr floatStruct = params->getFloatLogicalBufferStruct();

	for (GpuLogicalIndexUseMap::const_iterator i = floatStruct->map.begin();
		i != floatStruct->map.end(); ++i)
	{
		if (i->second.variability & mask)
		{
			UINT32 logicalIndex = i->first;
			const float* pFloat = params->getFloatPointer(i->second.physicalIndex);
			// Iterate over the params, set in 4-float chunks (low-level)
			for (UINT32 j = 0; j < i->second.currentSize; j+=4)
			{
				glProgramLocalParameter4fvARB(type, logicalIndex, pFloat);
				pFloat += 4;
				++logicalIndex;
			}
		}
	}
}

void GLArbGpuProgram::unloadImpl(void)
{
    glDeleteProgramsARB(1, &mProgramID);
}

void GLArbGpuProgram::loadFromSource(void)
{
    if (GL_INVALID_OPERATION == glGetError()) {
		// TODO LOG PORT LOG - Log this somewhere
        //LogManager::getSingleton().logMessage("Invalid Operation before loading program "+mName);

    }
    glBindProgramARB(mProgramType, mProgramID);
    glProgramStringARB(mProgramType, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)mSource.length(), mSource.c_str());

    if (GL_INVALID_OPERATION == glGetError())
    {
        GLint errPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
		String errPosStr = toString(errPos);
        char* errStr = (char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);
        // XXX New exception code?
		CM_EXCEPT(InternalErrorException, 
            "Cannot load GL vertex program.  Line " + errPosStr + ":\n" + errStr);
    }
    glBindProgramARB(mProgramType, 0);
}

