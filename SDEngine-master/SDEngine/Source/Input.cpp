#include"Input.h"


Input::Input(HINSTANCE hInstance, HWND hwnd, int screenWidth, int screenHeight)
{
	Initialize(hInstance, hwnd, screenWidth, screenHeight);
}

Input::Input(const Input& inputclass)
{

}

Input::~Input()
{

}


bool Input::Initialize(HINSTANCE hInstance, HWND hwnd, int screenWidth, int screenHeight)
{

	//ÖÃ¿ÕÖ¸Õë
	mDirectInput = NULL;
	mDirectInputKeyboard = NULL;
	mDirectInputMouse = NULL;


	//³õÊ¼»¯mScreenWidth,mScreenHeight
	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;
    
	//³õÊ¼»¯mMosuePosX  mMousePosY
	mMousePosX = 0;
	mMousePosY = 0;

	//µÚÒ»,-------------´´½¨DirectInput½Ó¿Ú,´Ë½Ó¿Ú¿ÉÒÔÓÃÀ´³õÊ¼»¯DirectInputDevice½Ó¿Ú------------------
	HR(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&mDirectInput, NULL));


	//µÚ¶þ,-------------ÓÃDirectInput½Ó¿ÚÀ´´´½¨DirectInputDevice½Ó¿Ú DirectInputKeyboard,²¢ÇÒÉèÖÃºÃ¸÷ÖÖ²ÎÊý----------------
	HR(mDirectInput->CreateDevice(GUID_SysKeyboard, &mDirectInputKeyboard, NULL));

	//ÉèÖÃ¼üÅÌÉè±¸½Ó¿ÚµÄÊý¾Ý¸ñÊ½(Ê¹ÓÃÏµÍ³Ô¤¶¨ÒåµÄ¸ñÊ½)
	HR(mDirectInputKeyboard->SetDataFormat(&c_dfDIKeyboard));

	//ÉèÖÃ¼üÅÌÉè±¸½Ó¿ÚµÄºÏ×÷µÈ¼¶,¼üÅÌÊäÈë²»ÓëÆäËü³ÌÐò·ÖÏí
	HR(mDirectInputKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE));

	//Èç¹û¼üÅÌÉèÖÃºÃÁË£¬»ñÈ¡¼üÅÌ¿ØÖÆÊ¹ÓÃÈ¨
	//HR(mDirectInputKeyboard->Acquire());

	//---------------µÚÈý, ÓÃDirectInput½Ó¿ÚÀ´´´½¨DirectInputDevice½Ó¿Ú DirectInputMouse,²¢ÇÒÉèÖÃºÃ¸÷ÖÖ²ÎÊý---------------------

	HR(mDirectInput->CreateDevice(GUID_SysMouse, &mDirectInputMouse, NULL));

	//ÉèÖÃÊó±êÉè±¸½Ó¿ÚµÄÊý¾Ý¸ñÊ½(Ê¹ÓÃÏµÍ³Ô¤¶¨ÒåµÄ¸ñÊ½)
	HR(mDirectInputMouse->SetDataFormat(&c_dfDIMouse));

	//ÉèÖÃÊó±êÉè±¸½Ó¿ÚµÄºÏ×÷µÈ¼¶,Êó±êÊäÈë²»ÓëÆäËü³ÌÐò·ÖÏí
 	HR(mDirectInputMouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE));

	//Èç¹ûÊó±êÉèÖÃºÃÁË£¬»ñÈ¡Êó±ê¿ØÖÆÊ¹ÓÃÈ¨
	//HR(mDirectInputMouse->Acquire());

	return TRUE;

}


void Input::ShutDown()
{
	//ÊÍ·ÅDirectInputMouse
	ReleaseCOM(mDirectInputMouse);

	//ÊÍ·ÅmDirectInputKeyboard
	ReleaseCOM(mDirectInputKeyboard);

	//ÊÍ·ÅDirectInput
	ReleaseCOM(mDirectInput);
}


bool Input::Frame()
{
	bool result;

	//¶ÁÈ¡¼üÅÌµÄÏÖÓÐ×´Ì¬
	result = ReadKeyboard();
	if (!result)
	{
		return false;
	}

	//¶ÁÈ¡Êó±êµÄÏÖÓÐ×´Ì¬
	result = ReadMouse();
	if (!result)
	{
		return false;
	}

	//´¦ÀíÔÚ¼üÅÌÄÇºÍÊó±êµÄ¸Ä±ä
	ProcessInput();
	return true;
}



bool Input::ReadKeyboard()
{
	HRESULT result;
	//¶ÁÈ¡¼üÅÌ×´Ì¬
	result = mDirectInputKeyboard->GetDeviceState(sizeof(mKeyboardState), (LPVOID)&mKeyboardState);

	if (FAILED(result))
	{
		//Èç¹ûÊÇ¼üÅÌÊ§È¥¼¯ÖÐµã»òÕßÎÞ·¨»ñÈ¡¿ØÖÆÈ¨,ÔòÖØÐÂ»ñÈ¡¿ØÖÆÈ¨
		if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))  //»òÕß²»ÊÇÇÒ
		{
			mDirectInputKeyboard->Acquire();
		}
		else
		{
			return false;
		}
	}
	return true;
}


bool Input::ReadMouse()
{
	HRESULT result;

	//¶ÁÈ¡Êó±ê×´Ì¬
	result = mDirectInputMouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mMouseState);
   
	if (FAILED(result))
	{
		//Èç¹ûÊÇÊó±êÊ§È¥¼¯ÖÐµã»òÕßÎÞ·¨»ñÈ¡¿ØÖÆÈ¨,ÔòÖØÐÂ»ñÈ¡¿ØÖÆÈ¨
		if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))//»òÕß²»ÊÇÇÒ
		{
			mDirectInputMouse->Acquire();
		}
		else
		{
			return false;
		}
	}
	return true;
}


void Input::ProcessInput()
{
	//¸üÐÂÊó±êµÄÆ«ÒÆÁ¿
	mMousePosXOffset = mMouseState.lX;
	mMousePosYOffset = mMouseState.lY;

	//¸üÐÂÊó±êµÄÎ»ÖÃ(Ã¿Ö¡¶¼½øÐÐ¸üÐÂ)
	mMousePosX += mMouseState.lX;
	mMousePosY += mMouseState.lY;

   //È·±£Êó±êÔÚÆÁÄ»Ö®ÄÚ
	if (mMousePosX < 0)
	{
		mMousePosX = 0;
	}
	if (mMousePosY < 0)
	{
		mMousePosY = 0;
	}
	if (mMousePosX > mScreenWidth)
	{
		mMousePosX = mScreenWidth;
	}
	if (mMousePosY > mScreenHeight)
	{
		mMousePosY = mScreenHeight;
	}
}

bool Input::IsEscapePressed()
{
	if (mKeyboardState[DIK_ESCAPE] & 0x80)
	{
		return true;
	}
		return false;
}

bool Input::IsWPressed()
{
	if (mKeyboardState[DIK_W] & 0x80)
	{
		return true;
	}
	return false;
}


bool Input::IsSPressed()
{
	if (mKeyboardState[DIK_S] & 0x80)
	{
		return true;
	}
	return false;
}



bool Input::IsAPressed()
{
	if (mKeyboardState[DIK_A] & 0x80)
	{
		return true;
	}
	return false;
}



bool Input::IsDPressed()
{
	if (mKeyboardState[DIK_D] & 0x80)
	{
		return true;
	}
	return false;
}



bool Input::IsQPressed()
{
	if (mKeyboardState[DIK_Q] & 0x80)
	{
		return true;
	}
	return false;
}



bool Input::IsEPressed()
{
	if (mKeyboardState[DIK_E] & 0x80)
	{
		return true;
	}
	return false;
}

//ËùÎ½µÄ°´ÏÂ¾ÍÊÇÇ°Ò»´Î´¦ÓÚup×´Ì¬£¬ÏÂÒ»´ÎÊÇdown×´Ì¬
bool Input::IsKeyDown(int key)
{
	if (mKeyboardState[key] & 0x80)
	{
		return true;
	}

	return false;
}

bool Input::IsMouseRightButtuonPressed()
{
	//Èç¹ûÓÒ¼ü°´ÏÂ
	if (mMouseState.rgbButtons[1] & 0x80)
	{
		return true;
	}
	return false;
}




void Input::GetMousePosition(int& MouseX, int& MouseY)
{
	MouseX = mMousePosX;
	MouseY = mMousePosY;
}



void Input::GetMousePositionOffset(int& MouseXOffset, int &MouseYOffset)
{
	MouseXOffset = mMousePosXOffset;
	MouseYOffset = mMousePosYOffset;
}