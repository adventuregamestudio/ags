package com.bigbluecup.android;

import java.util.TreeMap;

import android.view.KeyEvent;

public enum KeyCode
{
	Key0(KeyEvent.KEYCODE_0, '0'),
	Key1(KeyEvent.KEYCODE_1, '1'),
	Key2(KeyEvent.KEYCODE_2, '2'),
	Key3(KeyEvent.KEYCODE_3, '3'),
	Key4(KeyEvent.KEYCODE_4, '4'),
	Key5(KeyEvent.KEYCODE_5, '5'),
	Key6(KeyEvent.KEYCODE_6, '6'),
	Key7(KeyEvent.KEYCODE_7, '7'),
	Key8(KeyEvent.KEYCODE_8, '8'),
	Key9(KeyEvent.KEYCODE_9, '9'),
	KeyA(KeyEvent.KEYCODE_A, 'A', true),
	KeyALower(KeyEvent.KEYCODE_A, 'a'),
	// KeyAltLeft(KeyEvent.KEYCODE_ALT_LEFT, 0),
	// KeyAltRight(KeyEvent.KEYCODE_ALT_RIGHT, 0),
	KeyApostrophe(KeyEvent.KEYCODE_APOSTROPHE, '\''),
	KeyAt(KeyEvent.KEYCODE_AT, '@'),
	KeyB(KeyEvent.KEYCODE_B, 'B', true),
	KeyBLower(KeyEvent.KEYCODE_B, 'b'),
	KeyBackspace(KeyEvent.KEYCODE_DEL, 8), // confusingly named, this represents BACKSPACE, not DELETE
	KeyBackHardKey(KeyEvent.KEYCODE_BACK, 0),
	KeyBackSlash(KeyEvent.KEYCODE_BACKSLASH, '\\'),
	KeyC(KeyEvent.KEYCODE_C, 'C', true),
	KeyCLower(KeyEvent.KEYCODE_C, 'c'),
	// KeyCall(KeyEvent.KEYCODE_CALL, 0),
	// KeyCamera(KeyEvent.KEYCODE_CAMERA, 0),
	// KeyClear(KeyEvent.KEYCODE_CLEAR, 0),
	KeyComma(KeyEvent.KEYCODE_COMMA, ','),
	KeyD(KeyEvent.KEYCODE_D, 'D', true),
	KeyDLower(KeyEvent.KEYCODE_D, 'd'),
	// KeyDPadCenter(KeyEvent.KEYCODE_DPAD_CENTER, 0),
	// KeyDPadDown(KeyEvent.KEYCODE_DPAD_DOWN, 0),
	// KeyDPadLeft(KeyEvent.KEYCODE_DPAD_LEFT, 0),
	// KeyDPadRight(KeyEvent.KEYCODE_DPAD_RIGHT, 0),
	// KeyDPadUp(KeyEvent.KEYCODE_DPAD_UP, 0),
	KeyE(KeyEvent.KEYCODE_E, 'E', true),
	KeyELower(KeyEvent.KEYCODE_E, 'e'),
	// KeyEndCall(KeyEvent.KEYCODE_ENDCALL, 0),
	KeyEnter(KeyEvent.KEYCODE_ENTER, 13),
	// KeyEnvelope(KeyEvent.KEYCODE_ENVELOPE, 0),
	KeyEquals(KeyEvent.KEYCODE_EQUALS, '='),
	// KeyExplorer(KeyEvent.KEYCODE_EXPLORER, 0),
	KeyF(KeyEvent.KEYCODE_F, 'F', true),
	KeyFLower(KeyEvent.KEYCODE_F, 'f'),
	KeyF1(0x1000 + 47, 0),
	KeyF2(0x1000 + 48, 0),
	KeyF3(0x1000 + 49, 0),
	KeyF4(0x1000 + 50, 0),
	KeyF5(0x1000 + 51, 0),
	KeyF6(0x1000 + 52, 0),
	KeyF7(0x1000 + 53, 0),
	KeyF8(0x1000 + 54, 0),
	KeyF9(0x1000 + 55, 0),
	KeyF10(0x1000 + 56, 0),
	KeyF11(0x1000 + 57, 0),
	KeyF12(0x1000 + 58, 0),
	// KeyFocus(KeyEvent.KEYCODE_FOCUS, 0),
	KeyG(KeyEvent.KEYCODE_G, 'G', true),
	KeyGLower(KeyEvent.KEYCODE_G, 'g'),
	KeyGrave(KeyEvent.KEYCODE_GRAVE, '`'),
	KeyH(KeyEvent.KEYCODE_H, 'H', true),
	KeyHLower(KeyEvent.KEYCODE_H, 'h'),
	// KeyHeadSetHook(KeyEvent.KEYCODE_HEADSETHOOK, 0),
	// ///////////KeyHome(KeyEvent.KEYCODE_HOME, 0),
	KeyI(KeyEvent.KEYCODE_I, 'I', true),
	KeyILower(KeyEvent.KEYCODE_I, 'i'),
	KeyJ(KeyEvent.KEYCODE_J, 'J', true),
	KeyJLower(KeyEvent.KEYCODE_J, 'j'),
	KeyK(KeyEvent.KEYCODE_K, 'K', true),
	KeyKLower(KeyEvent.KEYCODE_K, 'k'),
	KeyL(KeyEvent.KEYCODE_L, 'L', true),
	KeyLLower(KeyEvent.KEYCODE_L, 'l'),
	KeyLeftBracket(KeyEvent.KEYCODE_LEFT_BRACKET, '['),
	KeyM(KeyEvent.KEYCODE_M, 'M', true),
	KeyMLower(KeyEvent.KEYCODE_M, 'm'),
	// KeyMediaFastForward(KeyEvent.KEYCODE_MEDIA_FAST_FORWARD, 0),
	// KeyMediaNext(KeyEvent.KEYCODE_MEDIA_NEXT, 0),
	// KeyMediaPlayPause(KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE, 0),
	// KeyMediaPrevious(KeyEvent.KEYCODE_MEDIA_PREVIOUS, 0),
	// KeyMediaRewind(KeyEvent.KEYCODE_MEDIA_REWIND, 0),
	// KeyMediaStop(KeyEvent.KEYCODE_MEDIA_STOP, 0),
	KeyMenuHardKey(KeyEvent.KEYCODE_MENU, 0),
	KeyMinus(KeyEvent.KEYCODE_MINUS, '-'),
	// KeyMute(KeyEvent.KEYCODE_MUTE, 0),
	KeyN(KeyEvent.KEYCODE_N, 'N', true),
	KeyNLower(KeyEvent.KEYCODE_N, 'n'),
	// KeyNotification(KeyEvent.KEYCODE_NOTIFICATION, 0),
	// KeyNum(KeyEvent.KEYCODE_NUM, 0),
	KeyO(KeyEvent.KEYCODE_O, 'O', true),
	KeyOLower(KeyEvent.KEYCODE_O, 'o'),
	KeyP(KeyEvent.KEYCODE_P, 'P', true),
	KeyPLower(KeyEvent.KEYCODE_P, 'p'),
	KeyPeriod(KeyEvent.KEYCODE_PERIOD, '.'),
	KeyPlus(KeyEvent.KEYCODE_PLUS, '+'),
	KeyPound(KeyEvent.KEYCODE_POUND, '#'),
	// KeyPower(KeyEvent.KEYCODE_POWER, 0),
	KeyQ(KeyEvent.KEYCODE_Q, 'Q', true),
	KeyQLower(KeyEvent.KEYCODE_Q, 'q'),
	KeyR(KeyEvent.KEYCODE_R, 'R', true),
	KeyRLower(KeyEvent.KEYCODE_R, 'r'),
	KeyRightBracket(KeyEvent.KEYCODE_RIGHT_BRACKET, ']'),
	KeyS(KeyEvent.KEYCODE_S, 'S', true),
	KeySLower(KeyEvent.KEYCODE_S, 's'),
	// KeySearch(KeyEvent.KEYCODE_SEARCH, 0),
	KeySemicolon(KeyEvent.KEYCODE_SEMICOLON, ';'),
	// KeyShiftLeft(KeyEvent.KEYCODE_SHIFT_LEFT, 0),
	// KeyShiftRight(KeyEvent.KEYCODE_SHIFT_RIGHT, 0),
	KeySlash(KeyEvent.KEYCODE_SLASH, '/'),
	// KeySoftLeft(KeyEvent.KEYCODE_SOFT_LEFT, 0),
	// KeySoftRight(KeyEvent.KEYCODE_SOFT_RIGHT, 0),
	KeySpace(KeyEvent.KEYCODE_SPACE, ' '),
	KeyStar(KeyEvent.KEYCODE_STAR, '*'),
	// KeySym(KeyEvent.KEYCODE_SYM, 0),
	KeyT(KeyEvent.KEYCODE_T, 'T', true),
	KeyTLower(KeyEvent.KEYCODE_T, 't'),
	KeyTab(KeyEvent.KEYCODE_TAB, 9),
	KeyU(KeyEvent.KEYCODE_U, 'U', true),
	KeyULower(KeyEvent.KEYCODE_U, 'u'),
	KeyUnknown(KeyEvent.KEYCODE_UNKNOWN, 0),
	KeyV(KeyEvent.KEYCODE_V, 'V', true),
	KeyVLower(KeyEvent.KEYCODE_V, 'v'),
	// KeyVolumeDown(KeyEvent.KEYCODE_VOLUME_DOWN, 0),
	// KeyVolumeUp(KeyEvent.KEYCODE_VOLUME_UP, 0),
	KeyW(KeyEvent.KEYCODE_W, 'W', true),
	KeyWLower(KeyEvent.KEYCODE_W, 'w'),
	KeyX(KeyEvent.KEYCODE_X, 'X', true),
	KeyXLower(KeyEvent.KEYCODE_X, 'x'),
	KeyY(KeyEvent.KEYCODE_Y, 'Y', true),
	KeyYLower(KeyEvent.KEYCODE_Y, 'y'),
	KeyZ(KeyEvent.KEYCODE_Z, 'Z', true),
	KeyZLower(KeyEvent.KEYCODE_Z, 'z');

	private static final TreeMap<Integer, KeyCode> androidKeyCodeMap;
	private static final TreeMap<Integer, KeyCode> unicodeCharMap;

	private final int androidKeyCode;
	private final int unicodeChar;
	private final boolean hasShift;

	private KeyCode(int androidKey, int unicode, boolean shift)
	{
		androidKeyCode = androidKey;
		unicodeChar = unicode;
		hasShift = shift;
	}

	private KeyCode(int androidKey, int unicode)
	{
		this(androidKey, unicode, false);
	}

	static
	{
		androidKeyCodeMap = new TreeMap<Integer, KeyCode>();
		unicodeCharMap = new TreeMap<Integer, KeyCode>();
		for (KeyCode key : KeyCode.values())
		{
			androidKeyCodeMap.put(key.androidKeyCode, key);
			if (key.unicodeChar > 0) unicodeCharMap.put(key.unicodeChar, key);
		}
		unicodeCharMap.put(0, KeyUnknown);
	}

	public int getAndroidKeyCode()
	{
		return androidKeyCode;
	}

	public int getUnicodeChar()
	{
		return unicodeChar;
	}

	public boolean hasShiftKey()
	{
		return hasShift;
	}

	public boolean isUpperCase()
	{
		return ((unicodeChar >= 'A') && (unicodeChar <= 'Z'));
	}

	public boolean isLowerCase()
	{
		return ((unicodeChar >= 'a') && (unicodeChar <= 'z'));
	}

	public boolean isAlpha()
	{
		return (isUpperCase() || isLowerCase());
	}

	public boolean isNumber()
	{
		return ((unicodeChar >= '0') && (unicodeChar <= '9'));
	}

	public KeyCode toUpper()
	{
		if (!isLowerCase()) return this;
		return KeyCode.findByUnicodeChar(unicodeChar - 32);
	}

	public KeyCode toLower()
	{
		if (!isUpperCase()) return this;
		return KeyCode.findByUnicodeChar(unicodeChar + 32);
	}

	public static KeyCode findByAndroidKeyCode(int androidKeyCode, boolean hasShift)
	{
		if (androidKeyCodeMap.containsKey(androidKeyCode))
		{
			KeyCode key = androidKeyCodeMap.get(androidKeyCode);
			if (hasShift) return key.toUpper();
			return key;
		}
		return KeyUnknown;
	}

	public static KeyCode findByUnicodeChar(int unicodeChar)
	{
		if (unicodeCharMap.containsKey(unicodeChar)) return unicodeCharMap.get(unicodeChar);
		return KeyUnknown;
	}
}
