#include <windows.h>
#include "d3dx9.h"

#include "XFileUtil.h"


//-----------------------------------------------------------------------------
// ���� ���� 

BOOL					g_bWoodTexture = TRUE;
LPDIRECT3D9             g_pD3D = NULL; // Direct3D ��ü 
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // ������ ��ġ (����ī��)

LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; // ���ؽ� ���� 
LPDIRECT3DVERTEXBUFFER9 g_pVBLight = NULL; // ����Ʈ�� ���ؽ� ���� 

PDIRECT3DVERTEXBUFFER9  g_pVBTexture = NULL; // �ؽ��� ��¿���ؽ� ����
LPDIRECT3DTEXTURE9      g_pTexture = NULL; // �ؽ��� �ε��� ����
LPDIRECT3DTEXTURE9      g_pFloorTexture = NULL; // �ٴ� �ؽ���
LPDIRECT3DTEXTURE9      g_pBulletTexture = NULL;

CXFileUtil g_XFile;
CXFileUtil g_XTank;

struct SPRITE {
	int spriteNumber;
	int curIndex;
	int frameCounter;
	int frameDelay;
	float x, y, z;
	BOOL state;
};

SPRITE g_Fire = { 15, 0, 0, 1, 0, 0, 0, FALSE };

struct POLAR {
	float x, y, z;
	float angle;
	float radius;
};

POLAR g_Camera = { 0, 3, -180, 90, 1.0 };

struct BULLET {
	float x, y, z;
	float deltaX, deltaZ;
	BOOL state;
};

BULLET g_Bullet = { 0, 0, 0, 0, 0, FALSE };

float g_TankX = 50.0f, g_TankZ = 50.0f; // �� ��ũ ��ġ


D3DXMATRIXA16 MatBillboardMatrix;
D3DXVECTOR3 g_vDir;

// Ŀ���� ���ؽ� Ÿ�� ����ü 
struct CUSTOMVERTEX
{
	FLOAT x, y, z;    // 3D ��ǥ��
	DWORD color;      // ���ؽ� ����
};

// Ŀ���� ���ؽ��� ������ ǥ���ϱ� ���� FVF(Flexible Vertex Format) �� 
// D3DFVF_XYZ(3D ���� ��ǥ) ��  D3DFVF_DIFFUSE(���� ����) Ư���� ��������.
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)

// ���� ó���� ���� ���ؽ� ����ü�� ���� ������� �ʴ´�
struct LIGHTVERTEX {
	D3DXVECTOR3 position;    // 3D ��ǥ ����ü 
	D3DXVECTOR3 normal;     // ���ؽ� �븻
};

// ���ؽ� ������ �����ϴ� FVF ����
#define D3DFVF_LIGHTVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL)

// �ؽ��� ��ǥ�� ������ ���ؽ� ����ü ����
struct TEXTUREVERTEX
{
	D3DXVECTOR3     position;  // ���ؽ��� ��ġ
	D3DCOLOR        color;     // ���ؽ��� ����
	FLOAT           tu, tv;    // �ؽ��� ��ǥ 
};

// �� ����ü�� ������ ǥ���ϴ� FVF �� ����
#define D3DFVF_TEXTUREVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)



//-----------------------------------------------------------------------------
// �̸�: InitD3D()
// ���: Direct3D �ʱ�ȭ, ���� �� �⺻ ���º��� �ʱ�ȭ 
//-----------------------------------------------------------------------------
HRESULT InitD3D(HWND hWnd)
{
	// Direct3D ��ü ���� 
	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		return E_FAIL;

	// ��ġ ������ ����Ÿ �غ�

	D3DPRESENT_PARAMETERS d3dpp;         // ��ġ ������ ���� ����ü ���� ����

	ZeroMemory(&d3dpp, sizeof(d3dpp));                  // ����ü Ŭ����
	d3dpp.BackBufferWidth = 1024;               // ���� �ػ� ���� ����
	d3dpp.BackBufferHeight = 768;               // ���� �ػ� ���� ���� 
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;   // ���� ���� ���� 
	d3dpp.BackBufferCount = 1;                 // ����� �� 
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;  // ���� ��� ����
	d3dpp.hDeviceWindow = hWnd;              // ������ �ڵ� ���� 
	d3dpp.Windowed = true;              // ������ ���� ���� �ǵ��� �� 
	d3dpp.EnableAutoDepthStencil = true;              // ���Ľ� ���۸� ����ϵ��� �� 
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;      // ���Ľ� ���� ���� ���� 

	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	// D3D��ü�� ��ġ ���� �Լ� ȣ�� (����Ʈ ����ī�� ���, HAL ���,
	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}
	// ���� ��ġ�� ���������� �����Ǿ���.

	// zbuffer ����ϵ��� ����
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);


	return S_OK;
}


//-----------------------------------------------------------------------------
// �̸�: InitGameData()
// ���: ���ӿ� ���õ� ���� �����͸� �ʱ�ȭ �Ѵ�. 
//-----------------------------------------------------------------------------
HRESULT InitGameData()
{
	g_XFile.XFileLoad(g_pd3dDevice, "./Images/skybox2.x");

	g_XTank.XFileLoad(g_pd3dDevice, "./Images/Dwarf.x");

	return S_OK;

}


//-----------------------------------------------------------------------------
// �̸�: InitGeometryTexture()
// ���: �ؽ��� ����� ���� ���ؽ� ���۸� ������ �� ���ؽ��� ä���. 
//-----------------------------------------------------------------------------
HRESULT InitGeometryTexture()
{
	// �ؽ��� �ε� 
	if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, "./Images/Fire.bmp", &g_pTexture)))
		return E_FAIL;
	
	if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, "./Images/seafloor.bmp", &g_pFloorTexture)))
	{
		return E_FAIL;
	}
	
	if (FAILED(D3DXCreateTextureFromFile(g_pd3dDevice, "./Images/Particle.dds", &g_pBulletTexture)))
	{
		return E_FAIL;
	}
	
	// ���ؽ� ���� ���� 
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(12 * sizeof(TEXTUREVERTEX), 0,
		D3DFVF_TEXTUREVERTEX, D3DPOOL_DEFAULT, &g_pVBTexture, NULL)))
	{
		return E_FAIL;
	}

	// ������ ���ؽ� ���� ���� 
	TEXTUREVERTEX* pVertices;
	if (FAILED(g_pVBTexture->Lock(0, 0, (void**)&pVertices, 0)))
		return E_FAIL;

	// ����
	pVertices[0].position = D3DXVECTOR3(-30, 50, 100);  // ���ؽ� ��ġ 
	pVertices[0].color = 0xffffffff;                  // ���ؽ� ���� �� ���� 
	pVertices[0].tu = 0.0f;                        // ���ؽ� U �ؽ��� ��ǥ 
	pVertices[0].tv = 0.0f;                        // ���ؽ� V �ؽ��� ��ǥ 

	pVertices[1].position = D3DXVECTOR3(30, 50, 100);
	pVertices[1].color = 0xffffffff;
	pVertices[1].tu = 64.0f/960.0f;
	pVertices[1].tv = 0.0f;

	pVertices[2].position = D3DXVECTOR3(-30, -10, 100);
	pVertices[2].color = 0xffffffff;
	pVertices[2].tu = 0.0f;
	pVertices[2].tv = 1.0f;

	pVertices[3].position = D3DXVECTOR3(30, -10, 100);
	pVertices[3].color = 0xffffffff;
	pVertices[3].tu = 64.0f / 960.0f;
	pVertices[3].tv = 1.0f;

	// �ٴ�
	pVertices[4].position = D3DXVECTOR3(-200, 0, 200);
	pVertices[4].color = 0xffffffff;
	pVertices[4].tu = 0.0f;
	pVertices[4].tv = 0.0f;

	pVertices[5].position = D3DXVECTOR3(200, 0, 200);
	pVertices[5].color = 0xffffffff;
	pVertices[5].tu = 10.0f;
	pVertices[5].tv = 0.0f;

	pVertices[6].position = D3DXVECTOR3(-200, 0, -200);
	pVertices[6].color = 0xffffffff;
	pVertices[6].tu = 0.0f;
	pVertices[6].tv = 10.0f;

	pVertices[7].position = D3DXVECTOR3(200, 0, -200);
	pVertices[7].color = 0xffffffff;
	pVertices[7].tu = 10.0f;
	pVertices[7].tv = 10.0f;

	// �Ѿ�
	pVertices[8].position = D3DXVECTOR3(-0.5, 2, 0);
	pVertices[8].color = 0xffffffff;
	pVertices[8].tu = 0.0f;
	pVertices[8].tv = 0.0f;

	pVertices[9].position = D3DXVECTOR3(0.5, 2, 0);
	pVertices[9].color = 0xffffffff;
	pVertices[9].tu = 1.0f;
	pVertices[9].tv = 0.0f;

	pVertices[10].position = D3DXVECTOR3(-0.5, 1, 0);
	pVertices[10].color = 0xffffffff;
	pVertices[10].tu = 0.0f;
	pVertices[10].tv = 1.0f;

	pVertices[11].position = D3DXVECTOR3(0.5, 1, 0);
	pVertices[11].color = 0xffffffff;
	pVertices[11].tu = 1.0f;
	pVertices[11].tv = 1.0f;
	
	g_pVBTexture->Unlock();

	return S_OK;
}


//-----------------------------------------------------------------------------
// �̸�: InitGeometry()
// ���: ���������� �ʱ�ȭ�Ѵ�.
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
	// ���ؽ� ���ۿ� ���� ���ؽ� �ڷḦ �ӽ÷� �����. 
	CUSTOMVERTEX vertices[] =
	{
		{ -200.0f,  0.0f, 0.0f, 0xff00ff00, }, // x�� ������ ���� ���ؽ� 
		{ 200.0f,  0.0f, 0.0f, 0xff00ff00, },

		{ 0.0f, 0.0f, -200.0f, 0xffffff00, },  // z�� ������ ���� ���ؽ�
		{ 0.0f, 0.0f,  200.0f, 0xffffff00, },

		{ 0.0f, -200.0f, 0.0f, 0xffff0000, },  // y�� ������ ���� ���ؽ�
		{ 0.0f,  200.0f, 0.0f, 0xffff0000, },

		{ 0.0f, 50.0f, 0.0f, 0xffff0000, },  // �ﰢ���� ù ��° ���ؽ� 
		{ -50.0f,  0.0f, 0.0f, 0xffff0000, },  // �ﰢ���� �� ��° ���ؽ� 
		{ 50.0f,  0.0f, 0.0f, 0xffff0000, },  // �ﰡ���� �� ��° ���ؽ� 
	};

	// ���ؽ� ���۸� �����Ѵ�.
	// �� ���ؽ��� ������ D3DFVF_CUSTOMVERTEX ��� �͵� ���� 
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(9 * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_DEFAULT, &g_pVB, NULL)))
	{
		return E_FAIL;
	}

	// ���ؽ� ���ۿ� ���� �� �� ���ؽ��� �ִ´�. 
	VOID* pVertices;
	if (FAILED(g_pVB->Lock(0, sizeof(vertices), (void**)&pVertices, 0)))
		return E_FAIL;
	memcpy(pVertices, vertices, sizeof(vertices));
	g_pVB->Unlock();

	return S_OK;
}


HRESULT InitGeometryLight()
{
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(6 * sizeof(LIGHTVERTEX), 
		0, D3DFVF_LIGHTVERTEX, D3DPOOL_DEFAULT, &g_pVBLight, NULL)))
	{
		return E_FAIL;
	}

	LIGHTVERTEX* pVertices;
	if (FAILED(g_pVBLight->Lock(0, 0, (void**)&pVertices, 0)))
		return E_FAIL;

	pVertices[0].position = D3DXVECTOR3(-30.0f, 0.0f, -30.0f);
	pVertices[1].position = D3DXVECTOR3(-60.0f, 30.0f, 0.0f);
	pVertices[2].position = D3DXVECTOR3(-90.0f, 0.0f, 30.0f);

	D3DXVECTOR3 p1 = pVertices[1].position - pVertices[0].position;
	D3DXVECTOR3 p2 = pVertices[2].position - pVertices[0].position;
	D3DXVECTOR3 pNormal;
	D3DXVec3Cross(&pNormal, &p2, &p1);

	pVertices[0].normal = pNormal;
	pVertices[1].normal = pNormal;
	pVertices[2].normal = pNormal;

	pVertices[3].position = D3DXVECTOR3(90.0f, 0.0f, 30.0f);
	pVertices[4].position = D3DXVECTOR3(60.0f, 30.0f, 0.0f);
	pVertices[5].position = D3DXVECTOR3(30.0f, 0.0f, -30.0f);

	p1 = pVertices[4].position - pVertices[3].position;
	p2 = pVertices[5].position - pVertices[3].position;
	pNormal;
	D3DXVec3Cross(&pNormal, &p2, &p1);

	pVertices[3].normal = pNormal;
	pVertices[4].normal = pNormal;
	pVertices[5].normal = pNormal;

	g_pVBLight->Unlock();

	return S_OK;

}


//-----------------------------------------------------------------------------
// �̸�: Cleanup()
// ���: �ʱ�ȭ�Ǿ��� ��� ��ü���� �����Ѵ�. 
//-----------------------------------------------------------------------------
VOID Cleanup()
{
	
	if (g_pVB != NULL)
		g_pVB->Release();
	if (g_pVBLight != NULL)
		g_pVBLight->Release();
	if (g_pVBTexture != NULL)
		g_pVBTexture->Release();
	if (g_pTexture != NULL)
		g_pTexture->Release();
		

	if (g_pd3dDevice != NULL)    // ��ġ ��ü ���� 
		g_pd3dDevice->Release();

	if (g_pD3D != NULL)          // D3D ��ü ���� 
		g_pD3D->Release();
}



//-----------------------------------------------------------------------------
// �̸�: SetupViewProjection()
// ���: �� ��ȯ�� �������� ��ȯ�� �����Ѵ�. 
//-----------------------------------------------------------------------------
VOID SetupViewProjection()
{
	// �� ��ȯ ���� 
	D3DXVECTOR3 vEyePt(g_Camera.x, g_Camera.y, g_Camera.z);    // ī�޶��� ��ġ 
	float destX = (float)(g_Camera.x + g_Camera.radius * cos(g_Camera.angle * (D3DX_PI / 180.0f)));
	float destZ = (float)(g_Camera.z + g_Camera.radius * sin(g_Camera.angle * (D3DX_PI / 180.0f)));
	D3DXVECTOR3 vLookatPt(destX, g_Camera.y, destZ);       // �ٶ󺸴� ���� 
	D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);          // ������ ���� 
	D3DXMATRIXA16 matView;                           // �亯ȯ�� ��Ʈ���� 
													 // �� ��Ʈ���� ���� 
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
	// Direct3D ��ġ�� �� ��Ʈ���� ���� 
	g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

	// ������� ��Ʈ����
	D3DXVECTOR3 vDir = vLookatPt - vEyePt;
	if (vDir.x > 0.0f)
		D3DXMatrixRotationY(&MatBillboardMatrix, -atanf(vDir.z / vDir.x) + D3DX_PI / 2);
	else
		D3DXMatrixRotationY(&MatBillboardMatrix, -atanf(vDir.z / vDir.x) - D3DX_PI / 2);
	g_vDir = vDir;

	// �������� ��ȯ ���� 
	D3DXMATRIXA16 matProj;   // �������ǿ� ��Ʈ���� 
							 // �������� ��Ʈ���� ���� 
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1024.0f/768.0f, 1.0f, 3000.0f);
	// Direct3D ��ġ�� �������� ��Ʈ���� ���� 
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

}

// ������ �̸� ���� ������ ���ϴ�.
D3DCOLORVALUE black = { 0, 0, 0, 1 };
D3DCOLORVALUE dark_gray = { 0.2f, 0.2f, 0.2f, 1.0f };
D3DCOLORVALUE gray = { 0.5f, 0.5f, 0.5f, 1.0f };
D3DCOLORVALUE red = { 1.0f, 0.0f, 0.0f, 1.0f };
D3DCOLORVALUE white = { 1.0f, 1.0f, 1.0f, 1.0f };
VOID SetupLight()
{
	D3DLIGHT9 light;                         // Direct3D 9 ���� ����ü ���� ����

	ZeroMemory(&light, sizeof(D3DLIGHT9));
	light.Type = D3DLIGHT_DIRECTIONAL;       // ���� Ÿ���� �𷺼ųη� ����
	light.Diffuse = white;                   // ������ �� ����
	light.Specular = white;

	D3DXVECTOR3 vecDir;
	vecDir = D3DXVECTOR3(10, -10, -5);
	D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDir);

	g_pd3dDevice->SetLight(0, &light);
	g_pd3dDevice->LightEnable(0, TRUE);

	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	g_pd3dDevice->SetRenderState(D3DRS_AMBIENT, 0x00808080);

}

HRESULT ChangeSpriteUV(SPRITE *sp)
{
	float u = (sp->curIndex * 64.0f) / 960.0f;
	float u2 = ((sp->curIndex + 1) * 64.0f) / 960.0f;

	TEXTUREVERTEX* pVertices;
	if (FAILED(g_pVBTexture->Lock(0, 0, (void**)&pVertices, 0)))
		return E_FAIL;

	pVertices[0].tu = u;
	pVertices[0].tv = 0.0f;

	pVertices[1].tu = u2;
	pVertices[1].tv = 0.0f;

	pVertices[2].tu = u;
	pVertices[2].tv = 1.0f;

	pVertices[3].tu = u2;
	pVertices[3].tv = 1.0f;

	g_pVBTexture->Unlock();

	if (sp->frameCounter >= sp->frameDelay) {
		sp->curIndex++;
		if (sp->curIndex == sp->spriteNumber)
			sp->state = FALSE;
	}
	else
		sp->frameCounter++;

	return S_OK;
}

VOID InputCheck()
{
	// ����
	if (GetAsyncKeyState(VK_UP)) {
		g_Camera.x += (float)(g_Camera.radius * cos(g_Camera.angle * (D3DX_PI / 180.0f)));
		g_Camera.z += (float)(g_Camera.radius * sin(g_Camera.angle * (D3DX_PI / 180.0f)));
	}
	//����
	if (GetAsyncKeyState(VK_DOWN)) {
		g_Camera.x -= (float)(g_Camera.radius * cos(g_Camera.angle * (D3DX_PI / 180.0f)));
		g_Camera.z -= (float)(g_Camera.radius * sin(g_Camera.angle * (D3DX_PI / 180.0f)));
	}
	// �·� ȸ��
	if (GetAsyncKeyState(VK_LEFT)) {
		g_Camera.angle += 1;
	}
	// ��� ȸ��
	if (GetAsyncKeyState(VK_RIGHT)) {
		g_Camera.angle -= 1;
	}

	// �Ѿ� �߻�
	if (GetAsyncKeyState(VK_SPACE)) {
		if (g_Bullet.state == FALSE) {
			g_Bullet.state = TRUE;
			g_Bullet.x = g_Camera.x;
			g_Bullet.y = g_Camera.y;
			g_Bullet.z = g_Camera.z;
			
			float speed = 5.0f;
			g_Bullet.deltaX = (float)(speed * cos(g_Camera.angle * (D3DX_PI / 180.0f)));
			g_Bullet.deltaZ = (float)(speed * sin(g_Camera.angle * (D3DX_PI / 180.0f)));
		}
	}

}

void BulletControl()
{
	if (g_Bullet.state == FALSE)
		return;

	g_Bullet.x += g_Bullet.deltaX;
	g_Bullet.z += g_Bullet.deltaZ;

	if (g_Bullet.x <= -200 || g_Bullet.x >= 200) {
		g_Bullet.state = FALSE;
		return;
	}

	if (g_Bullet.z <= -200 || g_Bullet.z >= 200) {
		g_Bullet.state = FALSE;
		return;
	}


	// �Ѿ˰� ��ũ �Ÿ� ���
	float distance = (float)sqrt((g_Bullet.x - g_TankX) * (g_Bullet.x - g_TankX) +
		(g_Bullet.z - g_TankZ) * (g_Bullet.z - g_TankZ));
	// ��ȣ �Ÿ� 40���� ������ �浹 ����
	if (distance < 40) {
		g_Bullet.state = FALSE; // �Ѿ� ����
		if (g_Fire.state == FALSE) { // ���� ��������Ʈ�� ��� ������ ���
			// ��������Ʈ �߻� ó��
			g_Fire.x = g_Bullet.x; // ��������Ʈ�� ��ġ ����
			g_Fire.y = g_Bullet.y;
			g_Fire.z = g_Bullet.z;
			g_Fire.frameDelay = 1;
			g_Fire.curIndex = 0;
			g_Fire.frameCounter = 0;
			g_Fire.state = TRUE;
		}
	}
}
/*
void DrawAxis()
{
	///// ���ؽ� ��� 
	// ���ؽ� ���� ���� 
	g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
	g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX); // ���ؽ� ���� ���� 

	D3DXMATRIXA16 matWorld;  // ���� ��ȯ�� ��Ʈ���� ���� 

	for (float x = -200; x <= 200; x += 20) {  // z �࿡ ������ ������ ���� �� �׸��� 
		D3DXMatrixTranslation(&matWorld, x, 0.0, 0.0);  // x�࿡ ���� ��ġ �̵� ��Ʈ����   
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld); // ��ȯ��Ʈ���� ���� 
		g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 2, 1);  // z�� ���� �׸��� 
	}

	for (float z = -200; z <= 200; z += 20) {  // x �࿡ ������ ������ ���� �� �׸��� 
		D3DXMatrixTranslation(&matWorld, 0.0, 0.0, z);  // z �࿡ ���� ��ġ �̵� ��Ʈ����
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);  // ��ȯ��Ʈ���� ���� 
		g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 0, 1);   // x�� ���� �׸��� 
	}

	D3DXMatrixIdentity(&matWorld);   // ��Ʈ������ ���� ��ķ� ���� 
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);  // ��ȯ ��Ʈ���� ���� 
	g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 4, 1);   // y �� �׸��� 
}


void DrawTextureRectangle()
{
	// ���� ����
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	// �ؽ��� ���� (�ؽ��� ������ ���Ͽ� g_pTexture�� ����Ͽ���.)
	g_pd3dDevice->SetTexture(0, g_pTexture);

	// �ؽ��� ��� ȯ�� ����
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	//g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	//g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 0X08);
	//g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

	// ����� ���ؽ� ���� ����
	g_pd3dDevice->SetStreamSource(0, g_pVBTexture, 0, sizeof(TEXTUREVERTEX));
	// FVF �� ����
	g_pd3dDevice->SetFVF(D3DFVF_TEXTUREVERTEX);
	// �簢�� ���� (�ﰢ�� 2���� �̿��Ͽ� �簢�� ������ �������) ��� 
	g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	// �ؽ��� ���� ����
	g_pd3dDevice->SetTexture(0, NULL);

	// �������� ��� alpha blending�� ������� �ʴ´�.
	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}
*/

//-----------------------------------------------------------------------------
// �̸�: Render()
// ���: ȭ���� �׸���.
//-----------------------------------------------------------------------------
VOID Render()
{
	if (NULL == g_pd3dDevice)  // ��ġ ��ü�� �������� �ʾ����� ���� 
		return;

	// �� �� �������� ��ȯ ����
	SetupViewProjection();

	// �ﰢ�� �ø� ��� ��
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// ���� �� (���� ���� �ƴϰ� ���ؽ� ��ü ���� ���)
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	
	// ����۸� ������ �������� �����.
	// ����۸� Ŭ����
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

	// ȭ�� �׸��� ���� 
	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		// ���ؽ� ���
		// ���ؽ� ���� ����
		g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

		D3DXMATRIXA16 matWorld;  // ���� ��ȯ�� ��Ʈ���� ���� 

		for (float x = -200; x <= 200; x += 20) {  // z �࿡ ������ ������ ���� �� �׸��� 
			D3DXMatrixTranslation(&matWorld, x, 0.0, 0.0);  // x�࿡ ���� ��ġ �̵� ��Ʈ����   
			g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld); // ��ȯ��Ʈ���� ���� 
			//g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 2, 1);  // z�� ���� �׸��� 
		}

		for (float z = -200; z <= 200; z += 20) {  // x �࿡ ������ ������ ���� �� �׸��� 
			D3DXMatrixTranslation(&matWorld, 0.0, 0.0, z);  // z �࿡ ���� ��ġ �̵� ��Ʈ����
			g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);  // ��ȯ��Ʈ���� ���� 
			//g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 0, 1);   // x�� ���� �׸��� 
		}

		D3DXMatrixIdentity(&matWorld);   // ��Ʈ������ ���� ��ķ� ���� 
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);  // ��ȯ ��Ʈ���� ���� 
		//g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 4, 1);   // y �� �׸��� 

		// ��ȯ�� ������� ���� ���¿��� �ﰢ�� �׸���
		//g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 6, 1);

		// matWorld�� x������ 100��ŭ �̵� ��ȯ ��Ʈ���� �����
		D3DXMatrixTranslation(&matWorld, 100.0, 0.0, 0.0);
		// ��Ʈ���� ���� �߰�
		D3DXMATRIXA16 matWorld2;
		// matworld2�� y���� �߽����� -45�� ȸ���ϴ� ��Ʈ���� �����
		D3DXMatrixRotationY(&matWorld2, -45.0f * (D3DX_PI / 180.0f));
		// matWorld�� �̵� �� ȸ���� �ϴ� ��Ʈ������ ������ִ� ���ϱ�
		D3DXMatrixMultiply(&matWorld, &matWorld2, &matWorld);
		// matWorld2�� y�� �������� 2�� Ȯ���ϴ� ��Ʈ���� �����
		D3DXMatrixScaling(&matWorld2, 2.0f, 1.0f, 1.0f);
		// matWorld�� Ȯ�� ��ɱ��� �߰��ϱ� ���� ���ϱ�
		D3DXMatrixMultiply(&matWorld, &matWorld2, &matWorld);
		// ���� ��ȯ�� ����� ���� ��Ʈ���� matWorld�� ��ġ�� ����
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

		//����Ʈ ����
		SetupLight();

		// ���� ����
		D3DMATERIAL9 mtrl; // ���� ������ ����ü

		ZeroMemory(&mtrl, sizeof(D3DMATERIAL9));
		// Ȯ�걤 �ֺ��� ������ ���� r g b a �� ����
		mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
		mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
		mtrl.Diffuse.b = mtrl.Ambient.b = 0.0f;
		mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
		// ������ ������ ����
		g_pd3dDevice->SetMaterial(&mtrl);

		// �ﰢ�� ����� ���� �̵�
		D3DXMatrixTranslation(&matWorld, 0, 0, -100);
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

		// �ﰢ�� ����� ���� ���ؽ� ���� ����
		g_pd3dDevice->SetStreamSource(0, g_pVBLight, 0, sizeof(LIGHTVERTEX));
		g_pd3dDevice->SetFVF(D3DFVF_LIGHTVERTEX); // ���ؽ� ���� ����
		//g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2); // �ﰢ�� 2�� �׸���
		
		////// x ���� ���
		
		// ��ī�� �ڽ� ���
		D3DXMatrixScaling(&matWorld, 60.0f, 60.0f, 60.0f);
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
		g_XFile.XFileDisplay(g_pd3dDevice);

		// �� ����� ���� ��Ʈ���� ��ȯ
		/*
		{
			D3DXMATRIXA16 matWorld;
			D3DXMatrixTranslation(&matWorld, 0, -2, 0);
			D3DXMATRIXA16 matWorld2;
			D3DXMatrixScaling(&matWorld2, 0.01f, 0.01f, 0.01f);
			D3DXMatrixMultiply(&matWorld, &matWorld2, &matWorld);
			g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
		}*/

		// �� ����ϱ�
		//g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		//g_XHouse.XFileDisplay(g_pd3dDevice);

		// ��ũ ����� ���� ��Ʈ���� ��ȯ
		{
			D3DXMATRIXA16 matWorld;
			D3DXMatrixTranslation(&matWorld, g_TankX, 0, g_TankZ);
			D3DXMATRIXA16 matWorld2;
			D3DXMatrixScaling(&matWorld2, 10.0f, 10.0f, 10.0f);
			D3DXMatrixMultiply(&matWorld, &matWorld2, &matWorld);
			D3DXMatrixRotationY(&matWorld2, (-90.0f * (D3DX_PI / 180.0f)));
			D3DXMatrixMultiply(&matWorld, &matWorld2, &matWorld);
			g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
		}

		// ��ũ ����ϱ�
		g_XTank.XFileDisplay(g_pd3dDevice);

		D3DXMatrixScaling(&matWorld, 1.0f, 1.0f, 1.0f);
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);


		// �ؽ��� ���

		// ���� ����
		g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

		// �ؽ��� ��� ȯ�� ����
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		// �ٴ� �ؽ��� ��� ����
		g_pd3dDevice->SetTexture(0, g_pFloorTexture);
		// ����� ���ؽ� ���� ����
		g_pd3dDevice->SetStreamSource(0, g_pVBTexture, 0, sizeof(TEXTUREVERTEX));
		// FVF �� ����
		g_pd3dDevice->SetFVF(D3DFVF_TEXTUREVERTEX);
		// �簢�� ���� ���
		g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 4, 2);


		// ���� �ؽ��� ��� ����
		
		if (g_Fire.state == TRUE) { // ������� ��츸 ���
			// �ؽ��� ����
			g_pd3dDevice->SetTexture(0, g_pTexture);

			// ���ؽ����� ���ļ¿� ���Ͽ� ���� ����
			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			{
				// �ش� ��ġ�� �̵� �� ������
				MatBillboardMatrix._41 = g_Fire.x;
				MatBillboardMatrix._42 = 0;
				MatBillboardMatrix._43 = g_Fire.z;
				g_pd3dDevice->SetTransform(D3DTS_WORLD, &MatBillboardMatrix);
			}

			// ����� ���ؽ� ���� ����
			g_pd3dDevice->SetStreamSource(0, g_pVBTexture, 0, sizeof(TEXTUREVERTEX));
			// FVF �� ����
			g_pd3dDevice->SetFVF(D3DFVF_TEXTUREVERTEX);
			// �簢�� ����
			g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			// ���� ��������Ʈ �ִϸ��̼��� ���� uv �ִϸ��̼� �Լ� ȣ��
			ChangeSpriteUV(&g_Fire);
		}

		// �Ѿ� �ؽ��� ��� (�Ѿ� ���� ����)
		if (g_Bullet.state == TRUE) {
			// ���ؽ����� ���ļ¿� ���Ͽ� ���� ����
			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			g_pd3dDevice->SetTexture(0, g_pBulletTexture);

			// ���� �ؽ��� ó�� �κ�
			g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 0x08);
			g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

			{
				// �ش� ��ġ�� �̵� �� ������
				MatBillboardMatrix._41 = g_Bullet.x;
				MatBillboardMatrix._42 = 0;
				MatBillboardMatrix._43 = g_Bullet.z;
				g_pd3dDevice->SetTransform(D3DTS_WORLD, &MatBillboardMatrix);
			}

			g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 8, 2);

			// �Ѿ� ��� �� ó��
			BulletControl();

		}
		

		// �ؽ��� ���� ����
		g_pd3dDevice->SetTexture(0, NULL);

		// ȭ�� �׸��� ��
		g_pd3dDevice->EndScene();
		

	}

	// ������� ������ ȭ������ ������. 
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}



//-----------------------------------------------------------------------------
// �̸� : MsgProc()
// ��� : ������ �޽��� �ڵ鷯 
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		Cleanup();   // ���α׷� ����� ��ü ������ ���Ͽ� ȣ���� 
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		Render();    // ȭ�� ����� ����ϴ� ������ �Լ� ȣ�� 
		ValidateRect(hWnd, NULL);
		return 0;
	

	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}



/**-----------------------------------------------------------------------------
* Ű���� �Է� ó��
*------------------------------------------------------------------------------
*/
/*
void ProcessKey(void)
{
	if (GetAsyncKeyState(VK_UP)) g_pCamera->MoveLocalZ(1.5f);	// ī�޶� ����!
	if (GetAsyncKeyState(VK_DOWN)) g_pCamera->MoveLocalZ(-1.5f);	// ī�޶� ����!
	if (GetAsyncKeyState(VK_LEFT)) g_pCamera->MoveLocalX(-1.5f);	// ī�޶� ����
	if (GetAsyncKeyState(VK_RIGHT)) g_pCamera->MoveLocalX(1.5f);	// ī�޶� ������

	if (GetAsyncKeyState('A')) g_pCamera->RotateLocalY(-.02f);
	if (GetAsyncKeyState('D')) g_pCamera->RotateLocalY(.02f);
	if (GetAsyncKeyState('W')) g_pCamera->RotateLocalX(-.02f);
	if (GetAsyncKeyState('S')) g_pCamera->RotateLocalX(.02f);

	D3DXMATRIXA16*	pmatView = g_pCamera->GetViewMatrix();		// ī�޶� ����� ��´�.
	g_pd3dDevice->SetTransform(D3DTS_VIEW, pmatView);			// ī�޶� ��� ����

	if (GetAsyncKeyState(' '))
		g_Tiger.AddVelocity(0, 0.5f, 0);
}

void Moving()
{
	ProcessKey();

	float y = g_pTerrain->getHeight(g_Tiger.GetPosition().x, g_Tiger.GetPosition().z);
	g_Tiger.SetGround(y);
	g_Tiger.Move();

}

void Action()
{
	Moving();
}
*/


//-----------------------------------------------------------------------------
// �̸�: WinMain()
// ���: ���α׷��� ������ 
//-----------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	// ������ Ŭ���� ���� ���� �� ���� 
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		"D3D Game", NULL };
	// ������ Ŭ���� ��� 
	RegisterClassEx(&wc);

	// ������ ���� 
	HWND hWnd = CreateWindow("D3D Game", "D3D Game Program",
		WS_OVERLAPPEDWINDOW, 100, 100, 800, 600,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);
	
	// Direct3D �ʱ�ȭ�� �����ϸ� �����ϰ�, �����ϸ� �����Ѵ�.
	if (SUCCEEDED(InitD3D(hWnd)) &&  // Derect3D�� �ʱ�ȭ ������
		SUCCEEDED(InitGeometry()) && // ���ؽ� ���� ���� ������
		SUCCEEDED(InitGeometryLight()) && // ����Ʈ ���ؽ� ���� ���� ������
		SUCCEEDED(InitGeometryTexture()) && // �ؽ��� ���ؽ� ���� ���� ������
		SUCCEEDED(InitGameData()) // ��Ÿ ���� ����Ÿ �ε�
		) // ���� �ε�
	{
		// ������ ���
		ShowWindow(hWnd, SW_SHOWDEFAULT);
		UpdateWindow(hWnd);

		// �޽��� ���� �����ϱ�
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));
		while (msg.message != WM_QUIT)
		{
			// �޽��ڰ� ������ ���� �´�. 
			if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else {
				Render();
				InputCheck();
			}
		}
		
	}

	UnregisterClass("D3D Game", wc.hInstance);

}
