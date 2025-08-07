#include <Windows.h>
#include <Winuser.h>
#include <Xinput.h>
#include <cmath>
#include <chrono>
#include <thread>
#include <iostream>

#pragma comment(lib, "Xinput.lib")

#define DELAY_TIME 10.0

#define LEFT_STICK_DEADZONE  0.0f
#define RIGHT_STICK_DEADZONE 0.0f

#define BASE_MOVE_SPEED  6.0f
#define BASE_X_SPEED  20.0f
#define BASE_B_SPEED  25.0f
#define BASE_A_SPEED  30.0f
#define BASE_LS_SPEED  15.0f
#define BASE_RESTRICT_MODE_SPEED  28.0f
#define BASE_RT_SPEED  40.0f
#define RIGHT_STICK_SPEED 220.0f
#define FAST_MODE_FACTOR 3.0f

#define DIAGONAL_THRESHOLD 1.5f
#define DIAGONAL_DELAY_FRAMES 20

void PreciseDelay(double milliseconds) {
	auto start = std::chrono::high_resolution_clock::now();
	while (true) {
		auto now = std::chrono::high_resolution_clock::now();
		double elapsed = std::chrono::duration<double, std::milli>(now - start).count();
		if (elapsed >= milliseconds) break;
		std::this_thread::yield();
	}
}

void SendKeyEvent(WORD key, bool keyDown) {
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = key;
	input.ki.dwFlags = keyDown ? 0 : KEYEVENTF_KEYUP;
	SendInput(1, &input, sizeof(INPUT));
}

void SendLeftClick(bool keyDown) {
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = keyDown ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
	SendInput(1, &input, sizeof(INPUT));
}

void SendRightClick(bool keyDown) {
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = keyDown ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
	SendInput(1, &input, sizeof(INPUT));
}

int main() {
	static float accumulated_dx = 0;
	static float accumulated_dy = 0;
	static float accumulated_right_dy = 0;

	XINPUT_STATE state;
	bool y_was_pressed = false;
	bool x_was_pressed = false;
	bool b_was_pressed = false;
	bool a_was_pressed = false;
	bool left_stick_was_pressed = false;
	bool lb_was_pressed = false;
	bool rt_was_pressed = false;

	std::cout << "BilliardsController 1.0.0 for Shanwan Q36 Pro XKV x Shooterspool\n";

	while (true) {
		auto loopStart = std::chrono::high_resolution_clock::now();

		if (XInputGetState(0, &state) != ERROR_SUCCESS) {
			Sleep(5);
			continue;
		}

		bool a_pressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A);
		bool y_pressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y);
		bool x_pressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X);
		bool b_pressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B);
		bool rb_pressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
		bool rt_pressed = (state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
		bool menu_pressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START);
		bool view_pressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK);
		bool left_stick_pressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
		bool lb_pressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);

		float current_base_speed;
		if (x_pressed) current_base_speed = BASE_X_SPEED;
		else if (b_pressed) current_base_speed = BASE_B_SPEED;
		else if (a_pressed) current_base_speed = BASE_A_SPEED;
		else if (left_stick_pressed) current_base_speed = BASE_LS_SPEED;
		else if (menu_pressed || view_pressed) current_base_speed = BASE_RESTRICT_MODE_SPEED;
		else if (rt_pressed) current_base_speed = BASE_RT_SPEED;
		else current_base_speed = BASE_MOVE_SPEED;

		float left_speed = current_base_speed;
		if (rb_pressed) left_speed *= FAST_MODE_FACTOR;

		if (y_pressed != y_was_pressed) SendKeyEvent('S', y_pressed);
		if (b_pressed != b_was_pressed) SendKeyEvent('B', b_pressed);
		if (a_pressed != a_was_pressed) SendLeftClick(a_pressed);
		if (left_stick_pressed != left_stick_was_pressed) SendKeyEvent('E', left_stick_pressed);
		if (lb_pressed != lb_was_pressed) SendRightClick(lb_pressed);
		if (rt_pressed != rt_was_pressed) SendKeyEvent('V', rt_pressed);

		if ((x_pressed || b_pressed || a_pressed || menu_pressed || view_pressed || left_stick_pressed || rt_pressed)
			&& !(menu_pressed && view_pressed))
		{
			float LX = state.Gamepad.sThumbLX / 32767.0f;
			float LY = state.Gamepad.sThumbLY / 32767.0f;
			float mag = sqrtf(LX * LX + LY * LY);

			if (mag > LEFT_STICK_DEADZONE) 
			{
				float adjMag = (mag - LEFT_STICK_DEADZONE) / (1.0f - LEFT_STICK_DEADZONE);
				float frame_dx = -LY * adjMag * left_speed;
				float frame_dy = -LX * adjMag * left_speed;

				float frame_mag = sqrtf(frame_dx * frame_dx + frame_dy * frame_dy);

				if (frame_mag >= DIAGONAL_THRESHOLD) 
				{
					INPUT input = { 0 };
					input.type = INPUT_MOUSE;
					input.mi.dx = !menu_pressed ? (LONG)(frame_dx) : 0;
					input.mi.dy = !view_pressed ? (LONG)(frame_dy) : 0;
					input.mi.dwFlags = MOUSEEVENTF_MOVE;
					SendInput(1, &input, sizeof(INPUT));

					accumulated_dx = 0;
					accumulated_dy = 0;
				}
				else 
				{
					accumulated_dx += frame_dx;
					accumulated_dy += frame_dy;

					float accum_mag = sqrtf(accumulated_dx * accumulated_dx + accumulated_dy * accumulated_dy);
					if (accum_mag >= DIAGONAL_THRESHOLD) 
					{
						INPUT input = { 0 };
						input.type = INPUT_MOUSE;
						input.mi.dx = !menu_pressed ? (LONG)(accumulated_dx) : 0;
						input.mi.dy = !view_pressed ? (LONG)(accumulated_dy) : 0;
						input.mi.dwFlags = MOUSEEVENTF_MOVE;
						SendInput(1, &input, sizeof(INPUT));

						accumulated_dx = 0;
						accumulated_dy = 0;
					}
				}
			}
			else 
			{
				if (accumulated_dx != 0 || accumulated_dy != 0) 
				{
					INPUT input = { 0 };
					input.type = INPUT_MOUSE;
					input.mi.dx = !menu_pressed ? (LONG)(accumulated_dx) : 0;
					input.mi.dy = !view_pressed ? (LONG)(accumulated_dy) : 0;
					input.mi.dwFlags = MOUSEEVENTF_MOVE;
					SendInput(1, &input, sizeof(INPUT));

					accumulated_dx = 0;
					accumulated_dy = 0;
				}
			}
		}

		if (y_pressed) {
			float RX = state.Gamepad.sThumbRX / 32767.0f;

			if (fabsf(RX) > RIGHT_STICK_DEADZONE) 
			{
				float adjRX = (fabsf(RX) - RIGHT_STICK_DEADZONE) / (1.0f - RIGHT_STICK_DEADZONE);
				float frame_speed = adjRX * RIGHT_STICK_SPEED * (RX > 0 ? 1 : -1);

				if (fabsf(frame_speed) >= DIAGONAL_THRESHOLD) 
				{
					INPUT input = { 0 };
					input.type = INPUT_MOUSE;
					input.mi.dy = (LONG)(-frame_speed);
					input.mi.dwFlags = MOUSEEVENTF_MOVE;
					SendInput(1, &input, sizeof(INPUT));

					accumulated_right_dy = 0;
				}
				else 
				{
					accumulated_right_dy += frame_speed;

					if (fabsf(accumulated_right_dy) >= DIAGONAL_THRESHOLD) {
						INPUT input = { 0 };
						input.type = INPUT_MOUSE;
						input.mi.dy = (LONG)(-accumulated_right_dy);
						input.mi.dwFlags = MOUSEEVENTF_MOVE;
						SendInput(1, &input, sizeof(INPUT));

						accumulated_right_dy = 0;
					}
				}
			}
			else if (fabsf(accumulated_right_dy) > 0) 
			{
				INPUT input = { 0 };
				input.type = INPUT_MOUSE;
				input.mi.dy = (LONG)(-accumulated_right_dy);
				input.mi.dwFlags = MOUSEEVENTF_MOVE;
				SendInput(1, &input, sizeof(INPUT));

				accumulated_right_dy = 0;
			}
		}

		y_was_pressed = y_pressed;
		x_was_pressed = x_pressed;
		b_was_pressed = b_pressed;
		a_was_pressed = a_pressed;
		left_stick_was_pressed = left_stick_pressed;
		lb_was_pressed = lb_pressed;
		rt_was_pressed = rt_pressed;

		PreciseDelay(DELAY_TIME);
		auto loopEnd = std::chrono::high_resolution_clock::now();
		auto loopTime = std::chrono::duration<double, std::milli>(loopEnd - loopStart).count();
		if (loopTime < DELAY_TIME) 
		{
			PreciseDelay(DELAY_TIME - loopTime);
		}
	}
	return 0;
}