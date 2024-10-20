///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{ 
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;

	InitializeShadowMapping();  // Call the method to initialize shadow mapping

	// Set up lighting, materials, or other initial configurations

	/*/ Initialize the light properties
	m_light.position = glm::vec3(0.0f, 0.0f, 4.0f); // Example light position
	m_light.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
	m_light.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
	m_light.specular = glm::vec3(1.0f, 1.0f, 1.0f);


	// Second Light Properties
	glm::vec3 light1_position = glm::vec3(-5.0f, 5.0f, 5.0f);  // Example: Light coming from the opposite side
	glm::vec3 light1_ambient = glm::vec3(0.1f, 0.1f, 0.1f);    // Dimmer ambient light
	glm::vec3 light1_diffuse = glm::vec3(0.7f, 0.7f, 0.7f);    // Bright white light, less intense than the main light
	glm::vec3 light1_specular = glm::vec3(1.0f, 1.0f, 1.0f);   // Strong specular highlights
	*/
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	glDeleteTextures(1, &shadowMap);  // Clean up texture
	glDeleteFramebuffers(1, &shadowMapFBO);  // Clean up framebuffer


}
void SceneManager::InitializeShadowMapping()
{
	glGenFramebuffers(1, &shadowMapFBO);
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		1024, 1024, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(false);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load( filename, &width, &height, &colorChannels, 0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor (0, 0, 0 ,1);

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/


	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.15f);
	goldMaterial.ambientStrength = 0.4f;
	goldMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.45f, 0.35f);
	goldMaterial.shininess = 30.0;
	goldMaterial.tag = "gold";
	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	cementMaterial.ambientStrength = 0.4f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.shininess = 0.3;
	cementMaterial.tag = "cement";
	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.06f, 0.04f, 0.12f); // to subtly increase ambient light absorption.
	woodMaterial.ambientStrength = 0.1f;
	woodMaterial.diffuseColor = glm::vec3(0.55f, 0.55f, 0.35f);
	woodMaterial.specularColor = glm::vec3(0.01f, 0.01f, 0.01f);
	woodMaterial.shininess =0.01;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.ambientColor = glm::vec3(0.25f, 0.35f, 0.45f);
	cementMaterial.ambientStrength = 0.4f;  
	tileMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	tileMaterial.specularColor = glm::vec3(0.5f, 0.4f, 0.3f);  
	tileMaterial.shininess = 15.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);  // Slightly brighter for better ambient light capture
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.7f, 0.6f, 0.5f);  // Make glass more reflective
	glassMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.shininess = 20.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.06f);
	clayMaterial.ambientStrength = 0.2f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.6f);
	clayMaterial.shininess = 5.0;
	clayMaterial.tag = "clay";
	m_objectMaterials.push_back(clayMaterial);

	OBJECT_MATERIAL clothMaterial;
	clothMaterial.ambientColor = glm::vec3(0.25f, 0.20f, 0.15f);  // a darker, richer color for cloth
	clothMaterial.ambientStrength = 0.05f;  // reduced ambient strength to mimic less reflective nature of cloth
	clothMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.1f);  // typical color for a fabric-like surface
	clothMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);  // very low specular highlights
	clothMaterial.shininess = 0.1;  // low shininess for a matte finish
	clothMaterial.tag = "cloth";  
	m_objectMaterials.push_back(clothMaterial);  

	OBJECT_MATERIAL paperMaterial;
	paperMaterial.ambientColor = glm::vec3(0.6f, 0.6f, 0.5f);  // light and neutral ambient color typical for paper
	paperMaterial.ambientStrength = 0.05f;  // moderate ambient strength to reflect ambient light well
	paperMaterial.diffuseColor = glm::vec3(0.08f, 0.1f, 0.1f);  // bright diffuse color for strong light reflection
	paperMaterial.specularColor = glm::vec3(0.3f, 0.1f, 0.1f);  // very subtle specular highlights
	paperMaterial.shininess = 0.5;  // very low shininess for a nearly matte finish
	paperMaterial.tag = "paper";  
	m_objectMaterials.push_back(paperMaterial);  


}
/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black 

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help  **/
	
	// Main Light (Brighter and more diffuse)
	m_pShaderManager->setVec3Value("lightSources[0].position", -8.0f, 30.0f, 30.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.5f, 0.5f, 0.1f); // Brighter diffuse light
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.7f, 0.6f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Additional Soft Fill Light (inside the flower vase)
	m_pShaderManager->setVec3Value("lightSources[1].position", 20.0f, 20.0f, -5.0f);
	//m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.1f, 0.1f);
	//m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.3f, 0.3f, 0.3f); // Softer light
	//m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.001f);
	//m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.3f);

	// Overhead Soft Light
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 0.0f, 10.0f);
	//m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.2f);
	//m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.2f, 0.2f, 0.2f);
	//m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 0.03f);
	//m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.4f);

	// FOURTH LIGHT SOURCE
	m_pShaderManager->setVec3Value("lightSources[3].position", -10.0f, -5.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.3f, 0.3f, 0.3f);
	//m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.1f, 0.1f, 0.1f);
	//m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.1f, 0.1f, 0.1f);  // Reduced to very soft highlights
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 0.01f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.1f);  // Minimized glare
	
	m_pShaderManager->setBoolValue("bUseLighting", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	
	// add and define the light sources for the scene
	SetupSceneLights();
	// define the materials for objects in the scene
	DefineObjectMaterials();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();

	
	// Load the texture
	glEnable(GL_TEXTURE_2D);
	CreateGLTexture("Resourses\\knife_handle.jpg", "floor");
	CreateGLTexture("Resourses\\body3.jpg", "candelbase");
	CreateGLTexture("Resourses\\body3.jpg", "candelbody");
	CreateGLTexture("Resourses\\silverbase.jpg", "base");
	CreateGLTexture("Resourses\\silverbase.jpg", "top");
	CreateGLTexture("Resourses\\vase.jpg", "vase");
	CreateGLTexture("Resourses\\alexa.jpg", "alexa");
	CreateGLTexture("Resourses\\harddrive.jpg", "drive");
	CreateGLTexture("Resourses\\bookcover.png", "book");
	CreateGLTexture("Resourses\\stainless_end.jpg", "basering");
	CreateGLTexture("Resourses\\stainless_end.jpg", "topring");
	CreateGLTexture("Resourses\\backdrop.jpg", "backdrop");
	CreateGLTexture("Resourses\\drywall.jpg", "drywall");

}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	glEnable(GL_LIGHTING); // Enable lighting for 3D rendering

	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*****************************************************/
	//background back-plane
	/*****************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 30.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.3, 0.4, 0.1, 1);
	// set the texture for the mesh
	SetShaderTexture("backdrop");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int backdropID = FindTextureID("backdrop");

	// Check if the texture ID was found
	if (backdropID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, backdropID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // Assuming this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);

	/*****************************************************/
	//background right-plane
	/*****************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(50.0f, 30.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(30.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1.0, 0.5, 0.3, 1);

	//SetShaderMaterial("cloth");
	// set the texture for the mesh
	SetShaderTexture("drywall");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int drywallID = FindTextureID("drywall");

	// Check if the texture ID was found
	if (drywallID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, drywallID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // Assuming this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);

	/*****************************************************/
	//ground plane "base"
	/*****************************************************/

	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(30.0f, 3.5f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

		
	// set the color values into the shader
	//SetShaderColor(0.4, 0.3, 0.4, 1);
	
	SetShaderMaterial("wood");

	// set the texture for the mesh
	SetShaderTexture("floor");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int floorID = FindTextureID("floor");

	// Check if the texture ID was found
	if (floorID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, floorID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // Assuming this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	// draw the mesh with transformation values - this plane is used for the base
	m_basicMeshes->DrawPlaneMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);


	/****************************************************************/
	// Torus mesh Candle-Bottom- ring
	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.5f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 85.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.0f, 0.2f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1, 1, 0, 1);

	// set the texture for the mesh
	SetShaderTexture("basering");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int baseringID = FindTextureID("basering");

	// Check if the texture ID was found
	if (baseringID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, baseringID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);
	
	/****************************************************************/
	// Tapered Cylinder mesh candel lowerbody
	/****************************************************************/
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 1.5f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.0f, 1.8f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1, 0.2, 0, 1);

	SetShaderMaterial("glass");

	// set the texture for the mesh
	SetShaderTexture("candelbase");
	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 
	// Retrieve the texture ID using its tag
	int candelbaseID = FindTextureID("candelbase");

	// Check if the texture ID was found
	if (candelbaseID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, candelbaseID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);
	

	/****************************************************************/
	// Cylinder mesh candel base
	/****************************************************************/

	// set the XYZ scale for the1mesh
	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.0f, 0.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.1, 0.4, 0.3, 1);
	SetShaderMaterial("clay");

	// set the texture for the mesh
	SetShaderTexture("base");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int baseID = FindTextureID("base");

	// Check if the texture ID was found
	if (candelbaseID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, baseID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);


	/****************************************************************/
	// Tapered Cylinder mesh candel upperbody
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 3.0f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.0f, 1.8f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0, 0, 1, 1);

	SetShaderMaterial("glass");
	// set the texture for the mesh
	SetShaderTexture("candelbase");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int candelbaseyID = FindTextureID("candelbase");

	// Check if the texture ID was found
	if (candelbaseID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, candelbaseID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);
	

	/****************************************************************/
	// Torus mesh-Candle Top ring
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.0f, 4.8f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1, 0.5, 0, 1);

	// set the texture for the mesh
	SetShaderTexture("topring");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int topringID = FindTextureID("topring");

	// Check if the texture ID was found
	if (topringID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, topringID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);


	/****************************************************************/
	//  Cylinder mesh Top
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.1f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.0f, 4.9f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(0.1, 0.4, 0.4, 1);
	
	SetShaderMaterial("clay");

	// set the texture for the mesh
	SetShaderTexture("top");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int topID = FindTextureID("top");

	// Check if the texture ID was found
	if (topID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, topID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	// Tapered Cylinder flower vase
	/****************************************************************/
	
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 20.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.0f, 20.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1, 0.2, 0, 1);

	SetShaderMaterial("glass");
	// set the texture for the mesh
	SetShaderTexture("vase");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int vaseID = FindTextureID("vase");

	// Check if the texture ID was found
	if (vaseID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, vaseID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);

	/****************************************************************/
	// Torus mesh white sphere  
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 210.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 110.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 1.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	//SetShaderColor(1, 1, 1, 1);


	SetShaderMaterial("cloth");

	// set the texture for the mesh
	SetShaderTexture("alexa");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 

	// Retrieve the texture ID using its tag
	int alexaID = FindTextureID("alexa");

	// Check if the texture ID was found
	if (alexaID != -1) {
		// Bind the wood texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, alexaID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}
	
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);

	/****************************************************************/
	//  Box1 Mesh black Hrad drive
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 3.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 1.75f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);

	SetShaderMaterial("glass");

	// set the texture for the mesh
	SetShaderTexture("drive");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 as an example

	// Retrieve the texture ID using its tag
	int driveID = FindTextureID("drive");

	// Check if the texture ID was found
	if (driveID != -1) {
		// Bind the texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, driveID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // Assuming this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	m_basicMeshes->DrawBoxMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);


	//***************************************************************/
	//  Box#2 Mesh the book
	/****************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.0f, 5.0f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.25f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("paper");

	// set the texture for the mesh
	SetShaderTexture("book");

	// Activating the texture 
	glActiveTexture(GL_TEXTURE0);  // Using texture unit 0 as an example

	// Retrieve the texture ID using its tag
	int bookID = FindTextureID("book");

	// Check if the texture ID was found
	if (bookID != -1) {
		// Bind the texture
		glActiveTexture(GL_TEXTURE0); // Using texture unit 0
		glBindTexture(GL_TEXTURE_2D, bookID);
		m_pShaderManager->setIntValue(g_TextureValueName, 0); // Assuming this sets the texture sampler uniform to 0
	}
	else {
		std::cerr << "texture not found!" << std::endl;
		// Handling the error 
	}

	m_basicMeshes->DrawBoxMesh();

	// Unbind the texture to avoid affecting other meshes
	glBindTexture(GL_TEXTURE_2D, 0);


	//std::cout << "Debug: I got here" << std::endl;
	// Disable lighting after drawing all objects
	glDisable(GL_LIGHTING);
}
