#pragma once

namespace MJ::Pad
{
	// Maschere dei pulsanti (identiche a XInput; i grilletti sono bit aggiunti da noi)
	constexpr std::uint32_t kUp = 0x0001;
	constexpr std::uint32_t kDown = 0x0002;
	constexpr std::uint32_t kLeft = 0x0004;
	constexpr std::uint32_t kRight = 0x0008;
	constexpr std::uint32_t kStart = 0x0010;
	constexpr std::uint32_t kBack = 0x0020;
	constexpr std::uint32_t kL3 = 0x0040;
	constexpr std::uint32_t kR3 = 0x0080;
	constexpr std::uint32_t kLB = 0x0100;
	constexpr std::uint32_t kRB = 0x0200;
	constexpr std::uint32_t kA = 0x1000;
	constexpr std::uint32_t kB = 0x2000;
	constexpr std::uint32_t kX = 0x4000;
	constexpr std::uint32_t kY = 0x8000;
	constexpr std::uint32_t kLT = 0x10000;
	constexpr std::uint32_t kRT = 0x20000;
	constexpr std::uint32_t kAll = 0x3F3FF;  // tutti i bit validi

	// Legge il controller (XInput, primo slot collegato). Restituisce la maschera dei
	// pulsanti premuti adesso, 0 se non c'e' nessun controller. Si puo' chiamare da
	// qualunque thread, piu' volte per frame.
	std::uint32_t Poll();

	// Stick sinistro "grezzo" (come lo vede XInput), da -1 a 1: x positivo = destra, y positivo = avanti,
	// con la zona morta standard gia' tolta. Serve perche' Skyrim in terza persona trasforma lo stick in
	// "avanti + rotazione" e non espone piu' la direzione laterale. Restituisce false se non c'e' un controller.
	bool LeftStick(float& a_x, float& a_y);

	bool Connected();    // c'e' un controller collegato?
	bool GameFocused();  // la finestra di Skyrim e' quella in primo piano?

	int         CountBits(std::uint32_t a_mask);
	std::string Describe(std::uint32_t a_mask);  // "LB + D-pad Destra", oppure "nessuno"
}
