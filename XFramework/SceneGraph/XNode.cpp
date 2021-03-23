
#include "XNode.h"

XNode::XNode()
{
	m_bVisiable = true;
	m_uRenderPathFlag = 0;
	m_fPosX = m_fPosY = m_fPosZ = 0.0f;
	m_fRotationX = m_fRotationY = m_fRotationZ = 0.0f;
	m_fScale = 1.0f;
}

void XNode::SetVisiable(bool bVisiable)
{
	m_bVisiable = bVisiable;
}
bool XNode::GetVisable()
{
	return m_bVisiable;
}
void XNode::SetRenderPathFlag(UINT uRenderPathFlag)
{
	m_uRenderPathFlag = uRenderPathFlag;
}
UINT XNode::GetRenderPathFlag()
{
	return m_uRenderPathFlag;
}

void XNode::SetPos(float x, float y, float z)
{
	m_fPosX = x;
	m_fPosY = y;
	m_fPosZ = z;
}
void XNode::GetPos(float& x, float& y, float& z)
{
	x = m_fPosX;
	y = m_fPosY;
	z = m_fPosZ;
}
void XNode::SetRotation(float x, float y, float z)
{
	m_fRotationX = x;
	m_fRotationY = y;
	m_fRotationZ = z;
}
void XNode::GetRotation(float& x, float& y, float& z)
{
	x = m_fRotationX;
	y = m_fRotationY;
	z = m_fRotationZ;
}
void XNode::SetScale(float s)
{
	m_fScale = s;
}
float XNode::GetScale()
{
	return m_fScale;
}