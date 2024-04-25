///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Joseph Les / Computer Science
//	Created for CS-330-Computational Graphics and Visualization
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
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

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
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
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
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"../../Utilities/textures/static3.jpg", "static"
	);

	bReturn = CreateGLTexture(
		"../../Utilities/textures/blackxbox4.jpg", "xbox"
	);

	bReturn = CreateGLTexture(
		"../../Utilities/textures/monster2.jpg", "monster"
	);

	bReturn = CreateGLTexture(
		"../../Utilities/textures/rusticwood.jpg", "rusticwood"
	);

	bReturn = CreateGLTexture(
		"../../Utilities/textures/blackwall.jpg", "wall"
	);

	bReturn = CreateGLTexture(
		"../../Utilities/textures/stainless.jpg", "stainless"
	);

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
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

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL cementMaterial;
	cementMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cementMaterial.ambientStrength = 0.2f;
	cementMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cementMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	cementMaterial.shininess = 0.5;
	cementMaterial.tag = "cement";

	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 90.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.3f);
	clayMaterial.ambientStrength = 0.3f;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	clayMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.4f);
	clayMaterial.shininess = 0.5;
	clayMaterial.tag = "clay";

	m_objectMaterials.push_back(clayMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// ENable custom lighting; the 3D scene will be black if no light sources are added
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Light source simulating sunlight coming from a window positioned in front, above and to the left
	glm::vec3 lightPosition(-5.0f, 10.0f, 5.0f);
	glm::vec3 lightDirection(0.5f, -1.0f, -0.5f);
	glm::vec3 color(1.5f, 1.4f, 0.9f);
	
	m_pShaderManager->setVec3Value("lightSources[0].position", lightPosition.x, lightPosition.y, lightPosition.z);
	m_pShaderManager->setVec3Value("lightSources[0].direction", lightDirection.x, lightDirection.y, lightDirection.z);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", color.x * 0.2f, color.y * 0.2f, color.z * 0.2f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", color.x, color.y, color.z);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", color.x, color.y, color.z);

	// Set focal strength and specular intensity to moderate values
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 100.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 1.0f);
	
	// 2nd light source
	glm::vec3 lightPosition2(5.0f, 10.0f, 5.0f);
	glm::vec3 lightDirection2(-0.5f, -1.0f, -0.5f);
	glm::vec3 secondColor(0.2f, 0.6f, 1.0f);


	m_pShaderManager->setVec3Value("lightSources[1].position", lightPosition2.x, lightPosition2.y, lightPosition2.z);
	m_pShaderManager->setVec3Value("lightSources[1].direction", lightDirection2.x, lightDirection2.y, lightDirection2.z);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", secondColor.x * 0.2f, secondColor.y * 0.2f, secondColor.z * 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", secondColor.x, secondColor.y, secondColor.z);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", secondColor.x, secondColor.y, secondColor.z);

	// Set focal strength and specular intensity to moderate values
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 100.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 1.0f);

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
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
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

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("rusticwood");
	SetShaderMaterial("clay");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	scaleXYZ = glm::vec3(20.0f, 0.0f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 8.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("wall");
	SetShaderMaterial("cement");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	glm::vec3 boxScale = glm::vec3(9.0f, 4.0f, 1.0f);
	glm::vec3 boxPosition = glm::vec3(0.0f, 4.5f, -9.0f);

	float boxRotationX = 0.0f;
	float boxRotationY = 0.0f;
	float boxRotationZ = 0.0f;
	
	SetTransformations(
		boxScale,
		boxRotationX,
		boxRotationY,
		boxRotationZ,
		boxPosition
	);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture("static");

	// Inner monitor
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	glm::vec3 boxMacScale = glm::vec3(2.0f, 1.0f, 1.0f);
	glm::vec3 boxMacPosition = glm::vec3(4.0f, 0.5f, -7.0f);

	float boxMacRotationX = 0.0f;
	float boxMacRotationY = 0.0f;
	float boxMacRotationZ = 0.0f;

	SetTransformations(
		boxMacScale,
		boxMacRotationX,
		boxMacRotationY,
		boxMacRotationZ,
		boxMacPosition
	);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture("stainless");

	// Inner monitor
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	glm::vec3 boxXScale = glm::vec3(2.0f, 5.0f, 1.0f);
	glm::vec3 boxXPosition = glm::vec3(-7.0f, 0.5f, -8.0f);

	float boxXRotationX = 180.0f;
	float boxXRotationY = 0.0f;
	float boxXRotationZ = 0.0f;

	SetTransformations(
		boxXScale,
		boxXRotationX,
		boxXRotationY,
		boxXRotationZ,
		boxXPosition
	);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	SetShaderTexture("xbox");

	// Inner monitor
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	glm::vec3 box2Scale = glm::vec3(10.0f, 5.0f, 1.0f);
	glm::vec3 box2Position = glm::vec3(0.0f, 4.5f, -9.0f);

	float box2RotationX = 0.0f;
	float box2RotationY = 0.0f;
	float box2RotationZ = 0.0f;

	SetTransformations(
		box2Scale,
		box2RotationX,
		box2RotationY,
		box2RotationZ,
		box2Position
	);

	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Monitor outline
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	glm::vec3 taperedCylinderScale = glm::vec3(0.7f, 2.0f, 0.2f);
	glm::vec3 taperedCylinderPosition = glm::vec3(0.0f, 0.0f, -9.0f);

	float taperedCylinderRotationX = -10.0f;
	float taperedCylinderRotationY = 0.0f;
	float taperedCylinderRotationZ = 0.0f;

	SetTransformations(
		taperedCylinderScale,
		taperedCylinderRotationX,
		taperedCylinderRotationY,
		taperedCylinderRotationZ,
		taperedCylinderPosition
	);

	SetShaderTexture("wall");
	SetShaderMaterial("cement");

	m_basicMeshes->DrawTaperedCylinderMesh();
	/****************************************************************/

	glm::vec3 prismScale = glm::vec3(6.0f, 0.8f, 0.3f);
	glm::vec3 prismPosition = glm::vec3(0.1f, 0.4f, -9.5f);

	float prismRotationX = 0.0f;
	float prismRotationY = 130.0f;
	float prismRotationZ = 0.0f;

	SetTransformations(
		prismScale,
		prismRotationX,
		prismRotationY,
		prismRotationZ,
		prismPosition
	);

	SetShaderTexture("wall");
	SetShaderMaterial("cement");

	m_basicMeshes->DrawPrismMesh();

	/****************************************************************/

	glm::vec3 prism2Scale = glm::vec3(6.0f, 0.8f, 0.3f);
	glm::vec3 prism2Position = glm::vec3(-0.1f, 0.4f, -9.5f);

	float prism2RotationX = 0.0f;
	float prism2RotationY = -130.0f;
	float prism2RotationZ = 0.0f;

	SetTransformations(
		prism2Scale,
		prism2RotationX,
		prism2RotationY,
		prism2RotationZ,
		prism2Position
	);

	SetShaderTexture("wall");
	SetShaderMaterial("cement");

	m_basicMeshes->DrawPrismMesh();

	/****************************************************************/

	glm::vec3 cylinderScale = glm::vec3(0.5f, 1.8f, 0.5f);
	glm::vec3 cylinderPosition = glm::vec3(-4.7f, 0.0f, -6.0f);

	float cylinderRotationX = -1.0f;
	float cylinderRotationY = 90.0f;
	float cylinderRotationZ = 0.0f;

	SetTransformations(
		cylinderScale,
		cylinderRotationX,
		cylinderRotationY,
		cylinderRotationZ,
		cylinderPosition
	);

	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("monster");
	SetShaderMaterial("glass");

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
}

