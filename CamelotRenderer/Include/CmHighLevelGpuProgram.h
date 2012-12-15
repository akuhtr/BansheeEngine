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
#ifndef __HighLevelGpuProgram_H__
#define __HighLevelGpuProgram_H__

#include "CmPrerequisites.h"
#include "CmGpuProgram.h"

namespace CamelotEngine {

	/** \addtogroup Core
	*  @{
	*/
	/** \addtogroup Resources
	*  @{
	*/
	/** Abstract base class representing a high-level program (a vertex or
        fragment program).
    @remarks
        High-level programs are vertex and fragment programs written in a high-level
        language such as Cg or HLSL, and as such do not require you to write assembler code
        like GpuProgram does. However, the high-level program does eventually 
        get converted (compiled) into assembler and then eventually microcode which is
        what runs on the GPU. As well as the convenience, some high-level languages like Cg allow
        you to write a program which will operate under both Direct3D and OpenGL, something
        which you cannot do with just GpuProgram (which requires you to write 2 programs and
        use each in a Technique to provide cross-API compatibility). Ogre will be creating
        a GpuProgram for you based on the high-level program, which is compiled specifically 
        for the API being used at the time, but this process is transparent.
    @par
        You cannot create high-level programs direct - use HighLevelGpuProgramManager instead.
        Plugins can register new implementations of HighLevelGpuProgramFactory in order to add
        support for new languages without requiring changes to the core Ogre API. To allow 
        custom parameters to be set, this class extends StringInterface - the application
        can query on the available custom parameters and get/set them without having to 
        link specifically with it.
    */
    class CM_EXPORT HighLevelGpuProgram : public GpuProgram
    {
    public:
        ~HighLevelGpuProgram();

		/**
		 * @brief	Initializes the gpu program. This must be called right after the program is constructed. 
		 * 			Called by HighLevelGpuManager upon creation, so usually you don't want to call this manually.
		 * 			
		 * @note	Initialization is not done immediately, and is instead just scheduled on the render thread.
		 */
		void initialize();

		/**
		 * @brief	Performs HighLevelGpuProgram initialization. Only callable from the render thread.
		 */
		virtual void initialize_internal();

		/**
		 * @copydoc GpuProgram::unload_internal()
		 */
		virtual void unload_internal();

        /** Creates a new parameters object compatible with this program definition. 
        @remarks
            Unlike low-level assembly programs, parameters objects are specific to the
            program and therefore must be created from it rather than by the 
            HighLevelGpuProgramManager. This method creates a new instance of a parameters
            object containing the definition of the parameters this program understands.
        */
        void createParameters_internal(AsyncOp& op);

        /** @copydoc GpuProgram::getBindingDelegate */
        GpuProgram* getBindingDelegate_internal(void) { return mAssemblerProgram.get(); }

		/** Get the full list of GpuConstantDefinition instances.
		@note
		Only available if this parameters object has named parameters.
		*/
		const GpuNamedConstants& getConstantDefinitions_internal() const;

    protected:
		/** Constructor, should be used only by factory classes. */
		HighLevelGpuProgram(const String& source, const String& entryPoint, const String& language, 
			GpuProgramType gptype, GpuProgramProfile profile, bool isAdjacencyInfoRequired = false);

        /// The underlying assembler program
        GpuProgramPtr mAssemblerProgram;
		/// Have we built the name->index parameter map yet?
		mutable bool mConstantDefsBuilt;

        /// Populate the passed parameters with name->index map
        virtual void populateParameterNames(GpuProgramParametersSharedPtr params);
		/** Build the constant definition map, must be overridden.
		@note The implementation must fill in the (inherited) mConstantDefs field at a minimum, 
			and if the program requires that parameters are bound using logical 
			parameter indexes then the mFloatLogicalToPhysical and mIntLogicalToPhysical
			maps must also be populated.
		*/
		virtual void buildConstantDefinitions() const = 0;

		/************************************************************************/
		/* 								STATICS		                     		*/
		/************************************************************************/
	public:
		static HighLevelGpuProgramPtr create(const String& source, const String& entryPoint, 
			const String& language, GpuProgramType gptype, GpuProgramProfile profile);
    };

	/** @} */

}
#endif
