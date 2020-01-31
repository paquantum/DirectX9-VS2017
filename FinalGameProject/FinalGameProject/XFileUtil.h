// XFileUtil.h: interface for the CXFileUtil class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XFILEUTIL_H__18859535_C36A_49F0_AE99_9D617B05A00C__INCLUDED_)
#define AFX_XFILEUTIL_H__18859535_C36A_49F0_AE99_9D617B05A00C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CXFileUtil
{
private:
	// x 파일 로드용 변수 
	LPD3DXMESH              g_pMesh;                 // 메쉬 객체 
	D3DMATERIAL9*           g_pMeshMaterials;        // 매쉬에 대한 재질 
	LPDIRECT3DTEXTURE9*     g_pMeshTextures;         // 매쉬에 대한 텍스쳐 
	DWORD                   g_dwNumMaterials;        // 재질의 수 


public:
	const LPD3DXMESH GetMesh() { return g_pMesh; };
	int XFileDisplay(LPDIRECT3DDEVICE9 pD3DDevice);
	int XFileLoad(LPDIRECT3DDEVICE9 pD3DDevice, char *xFileName);
	CXFileUtil();
	virtual ~CXFileUtil();

};

#endif // !defined(AFX_XFILEUTIL_H__18859535_C36A_49F0_AE99_9D617B05A00C__INCLUDED_)
