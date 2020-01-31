#include <windows.h>
#include "d3dx9.h"

#include "XFileUtil.h"


//-----------------------------------------------------------------------------
// 전역 변수 

BOOL					g_bWoodTexture = TRUE;
LPDIRECT3D9             g_pD3D = NULL; // Direct3D 객체 
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // 랜더링 장치 (비디오카드)

LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; // 버텍스 버퍼 
LPDIRECT3DVERTEXBUFFER9 g_pVBLight = NULL; // 라이트용 버텍스 버퍼 

PDIRECT3DVERTEXBUFFER9  g_pVBTexture = NULL; // 텍스쳐 출력용버텍스 버퍼
LPDIRECT3DTEXTURE9      g_pTexture = NULL; // 텍스쳐 로딩용 변수
LPDIRECT3DTEXTURE9      g_pFloorTexture = NULL; // 바닥 텍스쳐
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

float g_TankX = 50.0f, g_TankZ = 50.0f; // 적 탱크 위치


D3DXMATRIXA16 MatBillboardMatrix;
D3DXVECTOR3 g_vDir;

// 커스텀 버텍스 타입 구조체 
struct CUSTOMVERTEX
{
	FLOAT x, y, z;    // 3D 좌표값
	DWORD color;      // 버텍스 색상
};

// 커스텀 버텍스의 구조를 표시하기 위한 FVF(Flexible Vertex Format) 값 
// D3DFVF_XYZ(3D 월드 좌표) 와  D3DFVF_DIFFUSE(점의 색상) 특성을 가지도록.
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)

// 조명 처리를 위한 버텍스 구조체는 현재 사용하지 않는다
struct LIGHTVERTEX {
	D3DXVECTOR3 position;    // 3D 좌표 구조체 
	D3DXVECTOR3 normal;     // 버텍스 노말
};

// 버텍스 구조를 지정하는 FVF 정의
#define D3DFVF_LIGHTVERTEX (D3DFVF_XYZ|D3DFVF_NORMAL)

// 텍스쳐 좌표를 가지는 버텍스 구조체 정의
struct TEXTUREVERTEX
{
	D3DXVECTOR3     position;  // 버텍스의 위치
	D3DCOLOR        color;     // 버텍스의 색상
	FLOAT           tu, tv;    // 텍스쳐 좌표 
};

// 위 구조체의 구조를 표현하는 FVF 값 정의
#define D3DFVF_TEXTUREVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)



//-----------------------------------------------------------------------------
// 이름: InitD3D()
// 기능: Direct3D 초기화, 조명 및 기본 상태변수 초기화 
//-----------------------------------------------------------------------------
HRESULT InitD3D(HWND hWnd)
{
	// Direct3D 객체 생성 
	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
		return E_FAIL;

	// 장치 생성용 데이타 준비

	D3DPRESENT_PARAMETERS d3dpp;         // 장치 생성용 정보 구조체 변수 선언

	ZeroMemory(&d3dpp, sizeof(d3dpp));                  // 구조체 클리어
	d3dpp.BackBufferWidth = 1024;               // 버퍼 해상도 넓이 설정
	d3dpp.BackBufferHeight = 768;               // 버퍼 해상도 높이 설정 
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;   // 버퍼 포맷 설정 
	d3dpp.BackBufferCount = 1;                 // 백버퍼 수 
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;  // 스왑 방법 설정
	d3dpp.hDeviceWindow = hWnd;              // 윈도우 핸들 설정 
	d3dpp.Windowed = true;              // 윈도우 모드로 실행 되도록 함 
	d3dpp.EnableAutoDepthStencil = true;              // 스탠실 버퍼를 사용하도록 함 
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;      // 스탠실 버퍼 포맷 설정 

	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	// D3D객체의 장치 생성 함수 호출 (디폴트 비디오카드 사용, HAL 사용,
	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}
	// 이제 장치가 정상적으로 생성되었음.

	// zbuffer 사용하도록 설정
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);


	return S_OK;
}


//-----------------------------------------------------------------------------
// 이름: InitGameData()
// 기능: 게임에 관련된 각종 데이터를 초기화 한다. 
//-----------------------------------------------------------------------------
HRESULT InitGameData()
{
	g_XFile.XFileLoad(g_pd3dDevice, "./Images/skybox2.x");

	g_XTank.XFileLoad(g_pd3dDevice, "./Images/Dwarf.x");

	return S_OK;

}


//-----------------------------------------------------------------------------
// 이름: InitGeometryTexture()
// 기능: 텍스쳐 출력을 위한 버텍스 버퍼를 생성한 후 버텍스로 채운다. 
//-----------------------------------------------------------------------------
HRESULT InitGeometryTexture()
{
	// 텍스쳐 로딩 
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
	
	// 버텍스 버퍼 생성 
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(12 * sizeof(TEXTUREVERTEX), 0,
		D3DFVF_TEXTUREVERTEX, D3DPOOL_DEFAULT, &g_pVBTexture, NULL)))
	{
		return E_FAIL;
	}

	// 나무의 버텍스 버퍼 설정 
	TEXTUREVERTEX* pVertices;
	if (FAILED(g_pVBTexture->Lock(0, 0, (void**)&pVertices, 0)))
		return E_FAIL;

	// 폭발
	pVertices[0].position = D3DXVECTOR3(-30, 50, 100);  // 버텍스 위치 
	pVertices[0].color = 0xffffffff;                  // 버텍스 알파 및 색상 
	pVertices[0].tu = 0.0f;                        // 버텍스 U 텍스쳐 좌표 
	pVertices[0].tv = 0.0f;                        // 버텍스 V 텍스쳐 좌표 

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

	// 바닥
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

	// 총알
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
// 이름: InitGeometry()
// 기능: 기하정보를 초기화한다.
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
	// 버텍스 버퍼에 넣을 버텍스 자료를 임시로 만든다. 
	CUSTOMVERTEX vertices[] =
	{
		{ -200.0f,  0.0f, 0.0f, 0xff00ff00, }, // x축 라인을 위한 버텍스 
		{ 200.0f,  0.0f, 0.0f, 0xff00ff00, },

		{ 0.0f, 0.0f, -200.0f, 0xffffff00, },  // z축 라인을 위한 버텍스
		{ 0.0f, 0.0f,  200.0f, 0xffffff00, },

		{ 0.0f, -200.0f, 0.0f, 0xffff0000, },  // y축 라인을 위한 버텍스
		{ 0.0f,  200.0f, 0.0f, 0xffff0000, },

		{ 0.0f, 50.0f, 0.0f, 0xffff0000, },  // 삼각형의 첫 번째 버텍스 
		{ -50.0f,  0.0f, 0.0f, 0xffff0000, },  // 삼각형의 두 번째 버텍스 
		{ 50.0f,  0.0f, 0.0f, 0xffff0000, },  // 삼가형의 세 번째 버텍스 
	};

	// 버텍스 버퍼를 생성한다.
	// 각 버텍스의 포맷은 D3DFVF_CUSTOMVERTEX 라는 것도 전달 
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(9 * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_DEFAULT, &g_pVB, NULL)))
	{
		return E_FAIL;
	}

	// 버텍스 버퍼에 락을 건 후 버텍스를 넣는다. 
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
// 이름: Cleanup()
// 기능: 초기화되었던 모든 객체들을 해제한다. 
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
		

	if (g_pd3dDevice != NULL)    // 장치 객체 해제 
		g_pd3dDevice->Release();

	if (g_pD3D != NULL)          // D3D 객체 해제 
		g_pD3D->Release();
}



//-----------------------------------------------------------------------------
// 이름: SetupViewProjection()
// 기능: 뷰 변환과 프로젝션 변환을 설정한다. 
//-----------------------------------------------------------------------------
VOID SetupViewProjection()
{
	// 뷰 변환 설정 
	D3DXVECTOR3 vEyePt(g_Camera.x, g_Camera.y, g_Camera.z);    // 카메라의 위치 
	float destX = (float)(g_Camera.x + g_Camera.radius * cos(g_Camera.angle * (D3DX_PI / 180.0f)));
	float destZ = (float)(g_Camera.z + g_Camera.radius * sin(g_Camera.angle * (D3DX_PI / 180.0f)));
	D3DXVECTOR3 vLookatPt(destX, g_Camera.y, destZ);       // 바라보는 지점 
	D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);          // 업벡터 설정 
	D3DXMATRIXA16 matView;                           // 뷰변환용 매트릭스 
													 // 뷰 매트릭스 설정 
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
	// Direct3D 장치에 뷰 매트릭스 전달 
	g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

	// 빌보드용 매트릭스
	D3DXVECTOR3 vDir = vLookatPt - vEyePt;
	if (vDir.x > 0.0f)
		D3DXMatrixRotationY(&MatBillboardMatrix, -atanf(vDir.z / vDir.x) + D3DX_PI / 2);
	else
		D3DXMatrixRotationY(&MatBillboardMatrix, -atanf(vDir.z / vDir.x) - D3DX_PI / 2);
	g_vDir = vDir;

	// 프로젝션 변환 설정 
	D3DXMATRIXA16 matProj;   // 프로젝션용 매트릭스 
							 // 프로젝션 매트릭스 설정 
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1024.0f/768.0f, 1.0f, 3000.0f);
	// Direct3D 장치로 프로젝션 매트릭스 전달 
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);

}

// 색상을 미리 정해 놓으면 편리하다.
D3DCOLORVALUE black = { 0, 0, 0, 1 };
D3DCOLORVALUE dark_gray = { 0.2f, 0.2f, 0.2f, 1.0f };
D3DCOLORVALUE gray = { 0.5f, 0.5f, 0.5f, 1.0f };
D3DCOLORVALUE red = { 1.0f, 0.0f, 0.0f, 1.0f };
D3DCOLORVALUE white = { 1.0f, 1.0f, 1.0f, 1.0f };
VOID SetupLight()
{
	D3DLIGHT9 light;                         // Direct3D 9 조명 구조체 변수 선언

	ZeroMemory(&light, sizeof(D3DLIGHT9));
	light.Type = D3DLIGHT_DIRECTIONAL;       // 조명 타입을 디렉셔널로 설정
	light.Diffuse = white;                   // 조명의 색 설정
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
	// 전진
	if (GetAsyncKeyState(VK_UP)) {
		g_Camera.x += (float)(g_Camera.radius * cos(g_Camera.angle * (D3DX_PI / 180.0f)));
		g_Camera.z += (float)(g_Camera.radius * sin(g_Camera.angle * (D3DX_PI / 180.0f)));
	}
	//후진
	if (GetAsyncKeyState(VK_DOWN)) {
		g_Camera.x -= (float)(g_Camera.radius * cos(g_Camera.angle * (D3DX_PI / 180.0f)));
		g_Camera.z -= (float)(g_Camera.radius * sin(g_Camera.angle * (D3DX_PI / 180.0f)));
	}
	// 좌로 회전
	if (GetAsyncKeyState(VK_LEFT)) {
		g_Camera.angle += 1;
	}
	// 우로 회전
	if (GetAsyncKeyState(VK_RIGHT)) {
		g_Camera.angle -= 1;
	}

	// 총알 발사
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


	// 총알과 탱크 거리 계산
	float distance = (float)sqrt((g_Bullet.x - g_TankX) * (g_Bullet.x - g_TankX) +
		(g_Bullet.z - g_TankZ) * (g_Bullet.z - g_TankZ));
	// 상호 거리 40보다 작으면 충돌 간주
	if (distance < 40) {
		g_Bullet.state = FALSE; // 총알 소지
		if (g_Fire.state == FALSE) { // 폭발 스프라이트가 사용 가능한 경우
			// 스프라이트 발생 처리
			g_Fire.x = g_Bullet.x; // 스프라이트의 위치 설정
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
	///// 버텍스 출력 
	// 버텍스 버퍼 지정 
	g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
	g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX); // 버텍스 포멧 지정 

	D3DXMATRIXA16 matWorld;  // 월드 변환용 매트릭스 선언 

	for (float x = -200; x <= 200; x += 20) {  // z 축에 평행한 라인을 여러 개 그리기 
		D3DXMatrixTranslation(&matWorld, x, 0.0, 0.0);  // x축에 따라 위치 이동 매트릭스   
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld); // 변환매트릭스 적용 
		g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 2, 1);  // z축 라인 그리기 
	}

	for (float z = -200; z <= 200; z += 20) {  // x 축에 평행한 라인을 여러 개 그리기 
		D3DXMatrixTranslation(&matWorld, 0.0, 0.0, z);  // z 축에 따라 위치 이동 매트릭스
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);  // 변환매트릭스 적용 
		g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 0, 1);   // x축 라인 그리기 
	}

	D3DXMatrixIdentity(&matWorld);   // 매트릭스를 단위 행렬로 리셋 
	g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);  // 변환 매트릭스 전달 
	g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 4, 1);   // y 축 그리기 
}


void DrawTextureRectangle()
{
	// 조명 중지
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	// 텍스쳐 설정 (텍스쳐 매핑을 위하여 g_pTexture를 사용하였다.)
	g_pd3dDevice->SetTexture(0, g_pTexture);

	// 텍스쳐 출력 환경 설정
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	//g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	//g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 0X08);
	//g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

	// 출력할 버텍스 버퍼 설정
	g_pd3dDevice->SetStreamSource(0, g_pVBTexture, 0, sizeof(TEXTUREVERTEX));
	// FVF 값 설정
	g_pd3dDevice->SetFVF(D3DFVF_TEXTUREVERTEX);
	// 사각형 영역 (삼각형 2개를 이용하여 사각형 영역을 만들었음) 출력 
	g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	// 텍스쳐 설정 해제
	g_pd3dDevice->SetTexture(0, NULL);

	// 나머지의 경우 alpha blending을 사용하지 않는다.
	g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}
*/

//-----------------------------------------------------------------------------
// 이름: Render()
// 기능: 화면을 그린다.
//-----------------------------------------------------------------------------
VOID Render()
{
	if (NULL == g_pd3dDevice)  // 장치 객체가 생성되지 않았으면 리턴 
		return;

	// 뷰 및 프로젝션 변환 설정
	SetupViewProjection();

	// 삼각형 컬링 기능 끔
	g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	// 조명 끔 (조명 색상 아니고 버텍스 자체 색상 사용)
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	
	// 백버퍼를 지정된 색상으로 지운다.
	// 백버퍼를 클리어
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0);

	// 화면 그리기 시작 
	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		// 버텍스 출력
		// 버텍스 버퍼 지정
		g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

		D3DXMATRIXA16 matWorld;  // 월드 변환용 매트릭스 선언 

		for (float x = -200; x <= 200; x += 20) {  // z 축에 평행한 라인을 여러 개 그리기 
			D3DXMatrixTranslation(&matWorld, x, 0.0, 0.0);  // x축에 따라 위치 이동 매트릭스   
			g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld); // 변환매트릭스 적용 
			//g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 2, 1);  // z축 라인 그리기 
		}

		for (float z = -200; z <= 200; z += 20) {  // x 축에 평행한 라인을 여러 개 그리기 
			D3DXMatrixTranslation(&matWorld, 0.0, 0.0, z);  // z 축에 따라 위치 이동 매트릭스
			g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);  // 변환매트릭스 적용 
			//g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 0, 1);   // x축 라인 그리기 
		}

		D3DXMatrixIdentity(&matWorld);   // 매트릭스를 단위 행렬로 리셋 
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);  // 변환 매트릭스 전달 
		//g_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 4, 1);   // y 축 그리기 

		// 변환이 적용되지 않은 상태에서 삼각형 그리기
		//g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 6, 1);

		// matWorld를 x축으로 100만큼 이동 변환 매트릭스 만들기
		D3DXMatrixTranslation(&matWorld, 100.0, 0.0, 0.0);
		// 매트릭스 변수 추가
		D3DXMATRIXA16 matWorld2;
		// matworld2를 y축을 중심으로 -45도 회전하는 매트릭스 만들기
		D3DXMatrixRotationY(&matWorld2, -45.0f * (D3DX_PI / 180.0f));
		// matWorld를 이동 후 회전을 하는 매트릭스로 만들어주는 곱하기
		D3DXMatrixMultiply(&matWorld, &matWorld2, &matWorld);
		// matWorld2를 y축 방향으로 2배 확대하는 매트릭스 만들기
		D3DXMatrixScaling(&matWorld2, 2.0f, 1.0f, 1.0f);
		// matWorld에 확대 기능까지 추가하기 위한 곱하기
		D3DXMatrixMultiply(&matWorld, &matWorld2, &matWorld);
		// 복합 변환이 적용된 최종 매트릭스 matWorld를 장치로 전달
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

		//라이트 설정
		SetupLight();

		// 재질 설정
		D3DMATERIAL9 mtrl; // 재질 설정용 구주체

		ZeroMemory(&mtrl, sizeof(D3DMATERIAL9));
		// 확산광 주변광 재질에 대해 r g b a 값 설정
		mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
		mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
		mtrl.Diffuse.b = mtrl.Ambient.b = 0.0f;
		mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
		// 설정한 재질을 적용
		g_pd3dDevice->SetMaterial(&mtrl);

		// 삼각형 출력을 위한 이동
		D3DXMatrixTranslation(&matWorld, 0, 0, -100);
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);

		// 삼각형 출력을 위한 버텍스 버퍼 지정
		g_pd3dDevice->SetStreamSource(0, g_pVBLight, 0, sizeof(LIGHTVERTEX));
		g_pd3dDevice->SetFVF(D3DFVF_LIGHTVERTEX); // 버텍스 포맷 지정
		//g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2); // 삼각형 2개 그리기
		
		////// x 파일 출력
		
		// 스카이 박스 출력
		D3DXMatrixScaling(&matWorld, 60.0f, 60.0f, 60.0f);
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
		g_XFile.XFileDisplay(g_pd3dDevice);

		// 집 출력을 위한 매트릭스 변환
		/*
		{
			D3DXMATRIXA16 matWorld;
			D3DXMatrixTranslation(&matWorld, 0, -2, 0);
			D3DXMATRIXA16 matWorld2;
			D3DXMatrixScaling(&matWorld2, 0.01f, 0.01f, 0.01f);
			D3DXMatrixMultiply(&matWorld, &matWorld2, &matWorld);
			g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);
		}*/

		// 집 출력하기
		//g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		//g_XHouse.XFileDisplay(g_pd3dDevice);

		// 탱크 출력을 위한 매트릭스 변환
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

		// 탱크 출력하기
		g_XTank.XFileDisplay(g_pd3dDevice);

		D3DXMatrixScaling(&matWorld, 1.0f, 1.0f, 1.0f);
		g_pd3dDevice->SetTransform(D3DTS_WORLD, &matWorld);


		// 텍스쳐 출력

		// 조명 중지
		g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

		// 텍스쳐 출력 환경 설정
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		// 바닥 텍스쳐 출력 시작
		g_pd3dDevice->SetTexture(0, g_pFloorTexture);
		// 출력할 버텍스 버퍼 설정
		g_pd3dDevice->SetStreamSource(0, g_pVBTexture, 0, sizeof(TEXTUREVERTEX));
		// FVF 값 설정
		g_pd3dDevice->SetFVF(D3DFVF_TEXTUREVERTEX);
		// 사각형 영역 출력
		g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 4, 2);


		// 폭발 텍스쳐 출력 시작
		
		if (g_Fire.state == TRUE) { // 사용중인 경우만 출력
			// 텍스쳐 설정
			g_pd3dDevice->SetTexture(0, g_pTexture);

			// 버텍스들의 알파셋에 대하여 블랜딩 설정
			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			{
				// 해당 위치로 이동 및 빌보딩
				MatBillboardMatrix._41 = g_Fire.x;
				MatBillboardMatrix._42 = 0;
				MatBillboardMatrix._43 = g_Fire.z;
				g_pd3dDevice->SetTransform(D3DTS_WORLD, &MatBillboardMatrix);
			}

			// 출력할 버텍스 버퍼 설정
			g_pd3dDevice->SetStreamSource(0, g_pVBTexture, 0, sizeof(TEXTUREVERTEX));
			// FVF 값 설정
			g_pd3dDevice->SetFVF(D3DFVF_TEXTUREVERTEX);
			// 사각형 영역
			g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			// 폭발 스프라이트 애니메이션을 위한 uv 애니메이션 함수 호출
			ChangeSpriteUV(&g_Fire);
		}

		// 총알 텍스쳐 출력 (총알 사용될 때만)
		if (g_Bullet.state == TRUE) {
			// 버텍스들의 알파셋에 대하여 블랜딩 설정
			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			g_pd3dDevice->SetTexture(0, g_pBulletTexture);

			// 투명 텍스쳐 처리 부분
			g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			g_pd3dDevice->SetRenderState(D3DRS_ALPHAREF, 0x08);
			g_pd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

			{
				// 해당 위치로 이동 및 빌보딩
				MatBillboardMatrix._41 = g_Bullet.x;
				MatBillboardMatrix._42 = 0;
				MatBillboardMatrix._43 = g_Bullet.z;
				g_pd3dDevice->SetTransform(D3DTS_WORLD, &MatBillboardMatrix);
			}

			g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 8, 2);

			// 총알 출력 후 처리
			BulletControl();

		}
		

		// 텍스쳐 설정 해제
		g_pd3dDevice->SetTexture(0, NULL);

		// 화면 그리기 끝
		g_pd3dDevice->EndScene();
		

	}

	// 백버퍼의 내용을 화면으로 보낸다. 
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}



//-----------------------------------------------------------------------------
// 이름 : MsgProc()
// 기능 : 윈도우 메시지 핸들러 
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		Cleanup();   // 프로그램 종료시 객체 해제를 위하여 호출함 
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		Render();    // 화면 출력을 담당하는 렌더링 함수 호출 
		ValidateRect(hWnd, NULL);
		return 0;
	

	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}



/**-----------------------------------------------------------------------------
* 키보드 입력 처리
*------------------------------------------------------------------------------
*/
/*
void ProcessKey(void)
{
	if (GetAsyncKeyState(VK_UP)) g_pCamera->MoveLocalZ(1.5f);	// 카메라 전진!
	if (GetAsyncKeyState(VK_DOWN)) g_pCamera->MoveLocalZ(-1.5f);	// 카메라 후진!
	if (GetAsyncKeyState(VK_LEFT)) g_pCamera->MoveLocalX(-1.5f);	// 카메라 왼쪽
	if (GetAsyncKeyState(VK_RIGHT)) g_pCamera->MoveLocalX(1.5f);	// 카메라 오른쪽

	if (GetAsyncKeyState('A')) g_pCamera->RotateLocalY(-.02f);
	if (GetAsyncKeyState('D')) g_pCamera->RotateLocalY(.02f);
	if (GetAsyncKeyState('W')) g_pCamera->RotateLocalX(-.02f);
	if (GetAsyncKeyState('S')) g_pCamera->RotateLocalX(.02f);

	D3DXMATRIXA16*	pmatView = g_pCamera->GetViewMatrix();		// 카메라 행렬을 얻는다.
	g_pd3dDevice->SetTransform(D3DTS_VIEW, pmatView);			// 카메라 행렬 셋팅

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
// 이름: WinMain()
// 기능: 프로그램의 시작점 
//-----------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	// 윈도우 클래스 변수 선언 및 설정 
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		"D3D Game", NULL };
	// 윈도우 클래스 등록 
	RegisterClassEx(&wc);

	// 윈도우 생성 
	HWND hWnd = CreateWindow("D3D Game", "D3D Game Program",
		WS_OVERLAPPEDWINDOW, 100, 100, 800, 600,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);
	
	// Direct3D 초기화에 성공하면 진행하고, 실패하면 종료한다.
	if (SUCCEEDED(InitD3D(hWnd)) &&  // Derect3D의 초기화 성공시
		SUCCEEDED(InitGeometry()) && // 버텍스 버퍼 생성 성공시
		SUCCEEDED(InitGeometryLight()) && // 라이트 버텍스 버퍼 생성 성공시
		SUCCEEDED(InitGeometryTexture()) && // 텍스쳐 버텍스 버퍼 생성 성공시
		SUCCEEDED(InitGameData()) // 기타 게임 데이타 로드
		) // 사운드 로드
	{
		// 윈도우 출력
		ShowWindow(hWnd, SW_SHOWDEFAULT);
		UpdateWindow(hWnd);

		// 메시지 루프 시작하기
		MSG msg;
		ZeroMemory(&msg, sizeof(msg));
		while (msg.message != WM_QUIT)
		{
			// 메시자가 있으면 가져 온다. 
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
