#include "colorshaderclass.h"

// ***Quelle***: http://www.rastertek.com/tutdx11.html


ColorShaderClass::ColorShaderClass()
{
	m_vertexShader = 0;
	m_pixelShader = 0;
	m_layout = 0;
	m_matrixBuffer = 0;
}


ColorShaderClass::ColorShaderClass(const ColorShaderClass& other)
{
}


ColorShaderClass::~ColorShaderClass()
{
}

//The Initialize function will call the initialization function for the shaders.We pass in the name of the HLSL shader files, in this tutorial they are named color.vs and color.ps.

bool ColorShaderClass::Initialize(ID3D11Device* device, HWND hwnd)
{
	bool result;


	// Initialize the vertex and pixel shaders.
	result = InitializeShader(device, hwnd, L"../Kollisionserkennung DirectX/VertexShader.hlsl", L"../Kollisionserkennung DirectX/PixelShader.hlsl");
	if (!result)
	{
		std::cout << "moep" << endl;
		return false;
	}

	return true;
}

//The Shutdown function will call the shutdown of the shader.

void ColorShaderClass::Shutdown()
{
	// Shutdown the vertex and pixel shaders as well as the related objects.
	ShutdownShader();

	return;
}

// Render will first set the parameters inside the shader using the SetShaderParameters function. Once the parameters are
// set it then calls RenderShader to draw the green triangle using the HLSL shader.

bool ColorShaderClass::Render(ID3D11DeviceContext* deviceContext, int indexCount, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix,
	XMMATRIX& projectionMatrix)
{
	bool result;


	// Set the shader parameters that it will use for rendering.
	result = SetShaderParameters(deviceContext, worldMatrix, viewMatrix, projectionMatrix);
	if (!result)
	{
		return false;
	}

	// Now render the prepared buffers with the shader.
	RenderShader(deviceContext, indexCount);

	return true;
}

// Now we will start with one of the more important functions to this tutorial which is called InitializeShader.
// This function is what actually loads the shader files and makes it usable to DirectX and the GPU. You will also
// see the setup of the layout and how the vertex buffer data is going to look on the graphics pipeline in the GPU.
// The layout will need the match the VertexType in the modelclass.h file as well as the one defined in the VertexShader.hlsl file.

bool ColorShaderClass::InitializeShader(ID3D11Device* device, HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename)
{
	HRESULT result;
	ID3D10Blob* errorMessage; // wird benutzt um beliebige "Length-Data" zurückzugeben
	// Blobs can be used as a data buffer, storing vertex, adjacency, and material information during mesh optimization and loading
	// operations. Also, these objects are used to return object code and error messages in APIs that compile vertex, geometry and pixel shaders.

	// also wird HLSL kompiliert und in den Blob kommt das was rauskommt
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;

	// D3D11_INPUT_ELEMENT_DESC: A description of a single element for the input-assembler stage.
	// dient vermutlich dazu, Daten in den Shader hineinzubekommen (nein in die Pipeline, also nur indirekt in den Shader)
	D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
	unsigned int numElements;

	// Describes a buffer resource.
	D3D11_BUFFER_DESC matrixBufferDesc;


	// Initialize the pointers this function will use to null.
	errorMessage = 0;
	vertexShaderBuffer = 0;
	pixelShaderBuffer = 0;

	// Here is where we compile the shader programs into buffers. We give it the name of the shader file, the name of the shader, the shader 
	// version(5.0 in DirectX 11), and the buffer to compile the shader into. If it fails compiling the shader it will put an error message
	// inside the errorMessage string which we send to another function to write out the error.If it still fails and there is no errorMessage
	// string then it means it could not find the shader file in which case we pop up a dialog box saying so.

	// Compile the vertex shader code, "ColorVertexShader": Name der Methode im Shader, die aufgerufen wird
	result = D3DCompileFromFile(vsFilename, NULL, NULL, "ColorVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&vertexShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, vsFilename);
		}
		// If there was  nothing in the error message then it simply could not find the shader file itself.
		else
		{
			MessageBox(hwnd, vsFilename, L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// Compile the pixel shader code, "ColorPixelShader": Name der Methode im Shader, die aufgerufen wird
	result = D3DCompileFromFile(psFilename, NULL, NULL, "ColorPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
		&pixelShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		// If the shader failed to compile it should have writen something to the error message.
		if (errorMessage)
		{
			OutputShaderErrorMessage(errorMessage, hwnd, psFilename);
		}
		// If there was nothing in the error message then it simply could not find the file itself.
		else
		{
			MessageBox(hwnd, psFilename, L"Missing Shader File", MB_OK);
		}

		return false;
	}

	// Once the vertex shader and pixel shader code has successfully compiled into buffers we then use those buffers to create the shader objects
	// themselves. We will use these pointers to interface with the vertex and pixel shader from this point forward.

	// Create the vertex shader from the buffer.
	// device ist der aus der d3dclass, der supermanager der ne menge sachen machen kann
	// Shader-Objekt wird in den m_vertexShader gefüllt
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader);
	if (FAILED(result))
	{
		return false;
	}

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader);
	if (FAILED(result))
	{
		return false;
	}

	// The next step is to create the layout of the vertex data that will be processed by the shader. As this shader uses a position and
	// color vector we need to create both in the layout specifying the size of both.The semantic name is the first thing to fill out in
	// the layout, this allows the shader to determine the usage of this element of the layout. As we have two different elements we use
	// POSITION for the first one and COLOR for the second.The next important part of the layout is the Format. For the position vector
	// we use DXGI_FORMAT_R32G32B32_FLOAT and for the color we use DXGI_FORMAT_R32G32B32A32_FLOAT.The final thing you need to pay attention
	// to is the AlignedByteOffset which indicates how the data is spaced in the buffer. For this layout we are telling it the first 12
	// bytes are position and the next 16 bytes will be color, AlignedByteOffset shows where each element begins. You can use
	// D3D11_APPEND_ALIGNED_ELEMENT instead of placing your own values in AlignedByteOffset and it will figure out the spacing for you.The
	// other settings I've made default for now as they are not needed in this tutorial.

	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	polygonLayout[0].SemanticName = "POSITION";
	polygonLayout[0].SemanticIndex = 0;
	polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT; // A three-component, 96-bit floating-point format that supports 32 bits per color channel.
	polygonLayout[0].InputSlot = 0;
	polygonLayout[0].AlignedByteOffset = 0;
	polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate = 0;

	polygonLayout[1].SemanticName = "COLOR";
	polygonLayout[1].SemanticIndex = 0;
	polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // A four-component, 128-bit floating-point format that supports 32 bits per channel including alpha.
	polygonLayout[1].InputSlot = 0;

	// Optional. Offset (in bytes) between each element. Use D3D11_APPEND_ALIGNED_ELEMENT for convenience to define the current element directly after the previous one, including any packing if necessary.
	polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate = 0;

	// Once the layout description has been setup we can get the size of it and then create the input layout using the D3D device. Also release the vertex
	// and pixel shader buffers since they are no longer needed once the layout has been created.

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// Create the vertex input layout.
	// aus dem Vertexshaderbuffer wurde also ein shaderobjekt und ein layout erzeugt
	result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
		vertexShaderBuffer->GetBufferSize(), &m_layout);
	if (FAILED(result))
	{
		return false;
	}

	// Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = 0;

	// The final thing that needs to be setup to utilize (=benutzen) the shader is the constant buffer. As you saw in the vertex shader we currently have
	// just one constant buffer so we only need to setup one here so we can interface with the shader.The buffer usage needs to be set to dynamic since we
	// will be updating it each frame.The bind flags indicate that this buffer will be a constant buffer.The cpu access flags need to match up with the usage
	// so it is set to D3D11_CPU_ACCESS_WRITE. Once we fill out the description we can then create the constant buffer interface and then use that to access
	// the internal variables in the shader using the function SetShaderParameters.

	// Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
	// D3D11_USAGE_DYNAMIC: A resource that is accessible by both the GPU(read only) and the CPU(write only).
	// A dynamic resource is a good choice for a resource that will be updated by the CPU at least once per frame.To update a dynamic resource, use a Map method.
	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // gibt write und read
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
	if (FAILED(result))
	{
		return false;
	}

	return true;
}
// ganz schön crazy Funktion o.O


// ShutdownShader releases the four interfaces that were setup in the InitializeShader function.

void ColorShaderClass::ShutdownShader()
{
	// Release the matrix constant buffer.
	if (m_matrixBuffer)
	{
		m_matrixBuffer->Release();
		m_matrixBuffer = 0;
	}

	// Release the layout.
	if (m_layout)
	{
		m_layout->Release();
		m_layout = 0;
	}

	// Release the pixel shader.
	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = 0;
	}

	// Release the vertex shader.
	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = 0;
	}

	return;
}

// The OutputShaderErrorMessage writes out error messages that are generating when compiling either vertex shaders or pixel shaders.

void ColorShaderClass::OutputShaderErrorMessage(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
{
	char* compileErrors;
	unsigned long long bufferSize, i;
	//ofstream fout;


	// Get a pointer to the error message text buffer.
	compileErrors = (char*)(errorMessage->GetBufferPointer());

	// Get the length of the message.
	bufferSize = errorMessage->GetBufferSize();

	// Open a file to write the error message to.
	//fout.open("shader-error.txt");

	// Write out the error message.
	for (i = 0; i < bufferSize; i++)
	{
		cout << compileErrors[i];
	}

	// Close the file.
	//fout.close();

	// Release the error message.
	errorMessage->Release();
	errorMessage = 0;

	// Pop a message up on the screen to notify the user to check the text file for compile errors.
	MessageBox(hwnd, L"Error compiling shader. Check console output for message.", shaderFilename, MB_OK);

	return;
}

// The SetShaderVariables function exists to make setting the global variables in the shader easier. 
// The matrices used in this function are created inside the Scene, after which this function
// is called to send them from there into the vertex shader during the Render function call.

bool ColorShaderClass::SetShaderParameters(ID3D11DeviceContext* deviceContext, XMMATRIX& worldMatrix, XMMATRIX& viewMatrix,
	XMMATRIX& projectionMatrix)
{
	HRESULT result;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	MatrixBufferType* dataPtr;
	unsigned int bufferNumber;

	// Make sure to transpose matrices before sending them into the shader, this is a requirement for DirectX 11.

	// Transpose the matrices to prepare them for the shader.
	// XMMatrixTranspose: Computes the transpose of a matrix
	worldMatrix = XMMatrixTranspose(worldMatrix);
	viewMatrix = XMMatrixTranspose(viewMatrix);
	projectionMatrix = XMMatrixTranspose(projectionMatrix);

	// Lock the m_matrixBuffer, set the new matrices inside it, and then unlock it.

	// Lock the constant buffer so it can be written to.
	result = deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	dataPtr = (MatrixBufferType*)mappedResource.pData;

	// Copy the matrices into the constant buffer.
	dataPtr->world = worldMatrix;
	dataPtr->view = viewMatrix;
	dataPtr->projection = projectionMatrix;

	// Unlock the constant buffer.
	deviceContext->Unmap(m_matrixBuffer, 0);

	// Now set the updated matrix buffer in the HLSL vertex shader.

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 0;

	// Finanly set the constant buffer in the vertex shader with the updated values.
	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);

	return true;
}

// RenderShader is the second function called in the Render function.SetShaderParameters is called before this to ensure the shader parameters are setup correctly.

// The first step in this function is to set our input layout to active in the input assembler. This lets the GPU know the format of the data in the vertex buffer.
// The second step is to set the vertex shader and pixel shader we will be using to render this vertex buffer. Once the shaders are set we render the triangle by
// calling the DrawIndexed DirectX 11 function using the D3D device context. Once this function is called it will render the green triangle.

void ColorShaderClass::RenderShader(ID3D11DeviceContext* deviceContext, int indexCount)
{
	// Set the vertex input layout.
	deviceContext->IASetInputLayout(m_layout);

	// Set the vertex and pixel shaders that will be used to render this triangle.
	deviceContext->VSSetShader(m_vertexShader, NULL, 0);
	deviceContext->PSSetShader(m_pixelShader, NULL, 0);

	// Render the triangle.
	// DrawIndexed: Draw indexed, non - instanced primitives.
	deviceContext->DrawIndexed(indexCount, 0, 0);

	return;
}
