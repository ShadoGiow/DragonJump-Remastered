#include "Menu.h"

#include "Core.h"
#include "Pad.h"
#include "Settings.h"

#include "SKSEMenuFramework.h"

namespace MJ::Menu
{
	namespace
	{
		using Key = ImGuiMCP::ImGuiKey;

		struct KeyPair
		{
			Key key;
			int vk;
		};

		// Tabella tasti ImGui <-> codici Windows (VK)
		const std::vector<KeyPair>& Table()
		{
			static const std::vector<KeyPair> t = [] {
				std::vector<KeyPair> v;
				auto add = [&](Key k, int vk) { v.push_back({ k, vk }); };
				add(ImGuiMCP::ImGuiKey_Tab, 0x09);
				add(ImGuiMCP::ImGuiKey_LeftArrow, 0x25);
				add(ImGuiMCP::ImGuiKey_RightArrow, 0x27);
				add(ImGuiMCP::ImGuiKey_UpArrow, 0x26);
				add(ImGuiMCP::ImGuiKey_DownArrow, 0x28);
				add(ImGuiMCP::ImGuiKey_PageUp, 0x21);
				add(ImGuiMCP::ImGuiKey_PageDown, 0x22);
				add(ImGuiMCP::ImGuiKey_Home, 0x24);
				add(ImGuiMCP::ImGuiKey_End, 0x23);
				add(ImGuiMCP::ImGuiKey_Insert, 0x2D);
				add(ImGuiMCP::ImGuiKey_Delete, 0x2E);
				add(ImGuiMCP::ImGuiKey_Backspace, 0x08);
				add(ImGuiMCP::ImGuiKey_Space, 0x20);
				add(ImGuiMCP::ImGuiKey_Enter, 0x0D);
				add(ImGuiMCP::ImGuiKey_LeftCtrl, 0xA2);
				add(ImGuiMCP::ImGuiKey_LeftShift, 0xA0);
				add(ImGuiMCP::ImGuiKey_LeftAlt, 0xA4);
				add(ImGuiMCP::ImGuiKey_RightCtrl, 0xA3);
				add(ImGuiMCP::ImGuiKey_RightShift, 0xA1);
				add(ImGuiMCP::ImGuiKey_RightAlt, 0xA5);
				for (int i = 0; i < 10; ++i) {
					add(static_cast<Key>(ImGuiMCP::ImGuiKey_0 + i), 0x30 + i);
				}
				for (int i = 0; i < 26; ++i) {
					add(static_cast<Key>(ImGuiMCP::ImGuiKey_A + i), 0x41 + i);
				}
				for (int i = 0; i < 12; ++i) {
					add(static_cast<Key>(ImGuiMCP::ImGuiKey_F1 + i), 0x70 + i);
				}
				add(ImGuiMCP::ImGuiKey_Apostrophe, 0xDE);
				add(ImGuiMCP::ImGuiKey_Comma, 0xBC);
				add(ImGuiMCP::ImGuiKey_Minus, 0xBD);
				add(ImGuiMCP::ImGuiKey_Period, 0xBE);
				add(ImGuiMCP::ImGuiKey_Slash, 0xBF);
				add(ImGuiMCP::ImGuiKey_Semicolon, 0xBA);
				add(ImGuiMCP::ImGuiKey_Equal, 0xBB);
				add(ImGuiMCP::ImGuiKey_LeftBracket, 0xDB);
				add(ImGuiMCP::ImGuiKey_Backslash, 0xDC);
				add(ImGuiMCP::ImGuiKey_RightBracket, 0xDD);
				add(ImGuiMCP::ImGuiKey_GraveAccent, 0xC0);
				add(ImGuiMCP::ImGuiKey_CapsLock, 0x14);
				for (int i = 0; i < 10; ++i) {
					add(static_cast<Key>(ImGuiMCP::ImGuiKey_Keypad0 + i), 0x60 + i);
				}
				add(ImGuiMCP::ImGuiKey_KeypadDecimal, 0x6E);
				add(ImGuiMCP::ImGuiKey_KeypadDivide, 0x6F);
				add(ImGuiMCP::ImGuiKey_KeypadMultiply, 0x6A);
				add(ImGuiMCP::ImGuiKey_KeypadSubtract, 0x6D);
				add(ImGuiMCP::ImGuiKey_KeypadAdd, 0x6B);
				add(ImGuiMCP::ImGuiKey_MouseMiddle, 0x04);
				add(ImGuiMCP::ImGuiKey_MouseX1, 0x05);
				add(ImGuiMCP::ImGuiKey_MouseX2, 0x06);
				return v;
			}();
			return t;
		}

		int ToVk(Key a_key)
		{
			for (const auto& p : Table()) {
				if (p.key == a_key) {
					return p.vk;
				}
			}
			return 0;
		}

		Key FromVk(int a_vk)
		{
			for (const auto& p : Table()) {
				if (p.vk == a_vk) {
					return p.key;
				}
			}
			return ImGuiMCP::ImGuiKey_None;
		}

		const ImGuiMCP::ImVec4 kRed{ 1.0f, 0.35f, 0.3f, 1.0f };
		const ImGuiMCP::ImVec4 kGreen{ 0.4f, 0.9f, 0.4f, 1.0f };
		const ImGuiMCP::ImVec4 kYellow{ 1.0f, 0.85f, 0.3f, 1.0f };

		void Tip(const char* a_text)
		{
			if (ImGuiMCP::IsItemHovered()) {
				ImGuiMCP::SetTooltip("%s", a_text);
			}
		}

		bool Check(const char* a_label, bool& a_value, const char* a_tip = nullptr)
		{
			const bool changed = ImGuiMCP::Checkbox(a_label, &a_value);
			if (a_tip) {
				Tip(a_tip);
			}
			if (changed) {
				Settings::Get().MarkDirty();
			}
			return changed;
		}

		bool SliderF(const char* a_label, float& a_value, float a_min, float a_max, const char* a_fmt, float a_default, const char* a_tip = nullptr)
		{
			bool changed = ImGuiMCP::SliderFloat(a_label, &a_value, a_min, a_max, a_fmt);
			if (a_tip) {
				Tip(a_tip);
			}
			ImGuiMCP::SameLine();
			ImGuiMCP::PushID(a_label);
			if (ImGuiMCP::Button("Predefinito")) {
				a_value = a_default;
				changed = true;
			}
			ImGuiMCP::PopID();
			if (changed) {
				Settings::Get().MarkDirty();
			}
			return changed;
		}

		// selettore di tasto (pressione prolungata o singola, a seconda dell'azione)
		void KeyPicker(const char* a_label, Key& a_binding, bool& a_inited, int& a_vkSetting, const char* a_note)
		{
			if (!a_inited) {
				a_binding = FromVk(a_vkSetting);
				a_inited = true;
			}
			const Key before = a_binding;
			if (ImGuiMCPComponents::KeyBindingPicker(a_label, &a_binding)) {
				const int vk = ToVk(a_binding);
				if (a_binding != ImGuiMCP::ImGuiKey_None && vk == 0) {
					a_binding = before;  // tasto non supportato (es. pulsanti del controller)
				} else {
					a_vkSetting = vk;
					Settings::Get().MarkDirty();
				}
			}
			if (a_note) {
				Tip(a_note);
			}
		}

		Key g_kSuper = ImGuiMCP::ImGuiKey_None;
		Key g_kDash = ImGuiMCP::ImGuiKey_None;
		Key g_kSlam = ImGuiMCP::ImGuiKey_None;
		Key g_kAirN = ImGuiMCP::ImGuiKey_None;
		Key g_kAirP = ImGuiMCP::ImGuiKey_None;
		Key g_kAirC = ImGuiMCP::ImGuiKey_None;
		Key g_kAirR = ImGuiMCP::ImGuiKey_None;
		bool g_iAirN = false;
		bool g_iAirP = false;
		bool g_iAirC = false;
		bool g_iAirR = false;
		Key g_kReset = ImGuiMCP::ImGuiKey_None;
		bool g_iReset = false;
		bool g_iSuper = false;
		bool g_iDash = false;
		bool g_iSlam = false;
		Key  g_kWitch1 = ImGuiMCP::ImGuiKey_None;
		Key  g_kWitch2 = ImGuiMCP::ImGuiKey_None;
		bool g_iWitch1 = false;
		bool g_iWitch2 = false;

		// ---- pulsanti del controller (premi e assegna) ----
		int           g_capture = -1;  // quale azione stiamo assegnando (0 super salto, 1 dash, 2 slam, 3 reset, 4-7 attacchi in salto, 8 uscita dalla strega volante), -1 nessuna
		bool          g_capArmed = false;
		std::uint32_t g_capPeak = 0;

		void PadPicker(int a_index, const char* a_id, int& a_mask, int a_default, const char* a_note)
		{
			ImGuiMCP::PushID(a_id);
			if (g_capture == a_index) {
				const std::uint32_t now = Pad::Poll();
				if (!g_capArmed) {
					if (now == 0) {
						g_capArmed = true;  // aspetto che tu abbia lasciato tutti i pulsanti
					}
				} else if (now != 0) {
					g_capPeak |= now;
				} else if (g_capPeak != 0) {
					if (Pad::CountBits(g_capPeak) <= 3) {
						a_mask = static_cast<int>(g_capPeak);
						Settings::Get().MarkDirty();
					}
					g_capture = -1;
					g_capPeak = 0;
					g_capArmed = false;
				}
			}

			if (g_capture == a_index) {
				if (!Pad::Connected()) {
					ImGuiMCP::TextColored(kRed, "Nessun controller rilevato.");
				}
				ImGuiMCP::TextColored(kYellow, "Premi sul controller il pulsante (o la combinazione) da usare, poi rilascia...");
				if (g_capPeak != 0) {
					ImGuiMCP::Text("%s", Pad::Describe(g_capPeak).c_str());
				}
				if (ImGuiMCP::Button("Annulla")) {
					g_capture = -1;
					g_capPeak = 0;
					g_capArmed = false;
				}
			} else {
				ImGuiMCP::Text("Controller: %s", Pad::Describe(static_cast<std::uint32_t>(a_mask)).c_str());
				ImGuiMCP::SameLine();
				if (ImGuiMCP::Button("Cambia")) {
					g_capture = a_index;
					g_capArmed = false;
					g_capPeak = 0;
				}
				ImGuiMCP::SameLine();
				if (ImGuiMCP::Button("Nessuno")) {
					a_mask = 0;
					Settings::Get().MarkDirty();
				}
				ImGuiMCP::SameLine();
				if (ImGuiMCP::Button("Predefinito")) {
					a_mask = a_default;
					Settings::Get().MarkDirty();
				}
			}
			if (a_note) {
				Tip(a_note);
			}
			ImGuiMCP::PopID();
		}

		void WarnSameKey()
		{
			const auto& s = Settings::Get();
			auto clash = [](int a, int b) { return a != 0 && a == b; };
			if (clash(s.keySuperJump, s.keyDash) || clash(s.keySuperJump, s.keySlam) || clash(s.keyDash, s.keySlam)) {
				ImGuiMCP::TextColored(kYellow, "Attenzione: due azioni usano lo stesso tasto.");
			}
			if (clash(s.padSuperJump, s.padDash) || clash(s.padSuperJump, s.padSlam) || clash(s.padDash, s.padSlam) ||
				clash(s.padReset, s.padSuperJump) || clash(s.padReset, s.padDash) || clash(s.padReset, s.padSlam)) {
				ImGuiMCP::TextColored(kYellow, "Attenzione: due azioni usano gli stessi pulsanti del controller.");
			}
			if (clash(s.keyReset, s.keySuperJump) || clash(s.keyReset, s.keyDash) || clash(s.keyReset, s.keySlam)) {
				ImGuiMCP::TextColored(kYellow, "Attenzione: il tasto di reset e' usato anche da un'altra azione.");
			}
		}

		// avvisi della pagina degli attacchi: stesso pulsante usato da due attacchi, o da uno dei movimenti
		void WarnAirAttackClash()
		{
			const auto& s = Settings::Get();
			const int   pads[4] = { s.padAirNormal, s.padAirPower, s.padAirCast, s.padAirRanged };
			const int   keys[4] = { s.keyAirNormal, s.keyAirPower, s.keyAirCast, s.keyAirRanged };
			const int   movePads[5] = { s.padSuperJump, s.padDash, s.padSlam, s.padReset, s.padWitchExit };
			const int   moveKeys[5] = { s.keySuperJump, s.keyDash, s.keySlam, s.keyReset, s.keyWitchExit };
			bool        sameAtk = false;
			bool        sameMove = false;
			for (int i = 0; i < 4; ++i) {
				for (int j = i + 1; j < 4; ++j) {
					// LT+RT contiene sia LT sia RT: non e' un conflitto, e' proprio la logica "premi prima uno, poi l'altro"
					if ((pads[i] != 0 && pads[i] == pads[j]) || (keys[i] != 0 && keys[i] == keys[j])) {
						sameAtk = true;
					}
				}
				for (int j = 0; j < 5; ++j) {
					if ((pads[i] != 0 && pads[i] == movePads[j]) || (keys[i] != 0 && keys[i] == moveKeys[j])) {
						sameMove = true;
					}
				}
			}
			if (sameAtk) {
				ImGuiMCP::TextColored(kYellow, "Attenzione: due attacchi usano lo stesso pulsante.");
			}
			if (sameMove) {
				ImGuiMCP::TextColored(kYellow, "Attenzione: un attacco usa lo stesso pulsante di dash, slam, super salto, reset o uscita dalla strega volante.");
			}
		}

		// -------------------------------------------------------------
		// pagine
		// -------------------------------------------------------------
		void __stdcall PageGeneral()
		{
			Core::MenuTouched();
			auto&       s = Settings::Get();
			const auto& st = Core::GetStatus();

			ImGuiMCP::SeparatorText("MJN");
			Check("Mod attiva", s.enabled, "Spenta = Skyrim torna normale (animazioni, gravita', salti).");
			Check("Messaggi a schermo", s.messages);
			Check("Usa il controller (gamepad)", s.padEnabled, "Legge i pulsanti del controller. I tasti della tastiera funzionano comunque.");
			if (s.padEnabled) {
				Pad::Poll();
				if (Pad::Connected()) {
					ImGuiMCP::TextColored(kGreen, "Controller rilevato.");
				} else {
					ImGuiMCP::TextColored(kYellow, "Nessun controller rilevato (si cerca ogni pochi secondi).");
				}
			}

			ImGuiMCP::Spacing();
			if (!st.formsOk) {
				ImGuiMCP::TextColored(kRed, "doublejump.esp non trovato o incompleto: la mod non puo' funzionare.");
				ImGuiMCP::TextWrapped("Controlla che nella lista dei plugin sia attivo il doublejump.esp di questo pacchetto (non una copia vecchia).");
			} else {
				ImGuiMCP::Text("Stato: %s", (st.state && *st.state) ? st.state : "pronta");
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Personaggio bloccato? Reset animazioni");
			ImGuiMCP::TextWrapped("Se dopo un salto, un dash o uno slam il personaggio resta bloccato o con l'animazione sbagliata, questo riporta tutto allo stato normale.");
			if (ImGuiMCP::Button("Reset animazioni personaggio")) {
				Core::RequestReset();
			}
			Tip("Riattiva i controlli, spegne bAnimationDriven e le variabili dei salti, rimette l'animazione di base e azzera dash/slam/salti in corso.");
			KeyPicker("Tasto di reset (tieni premuto)##reset", g_kReset, g_iReset, s.keyReset, "Tieni premuto per il tempo indicato qui sotto. Nessuno di default.");
			PadPicker(3, "padreset", s.padReset, 0x0320, "Tieni premuti insieme questi pulsanti sul controller. Di default LB + RB + Back.");
			SliderF("Tempo di pressione (secondi)##reset", s.resetHold, 0.2f, 3.0f, "%.2f", 0.6f, "Per quanto tieni premuto prima che il reset parta (evita i reset involontari).");
			WarnSameKey();
			ImGuiMCP::TextWrapped("Il reset NON parte mai da solo (neanche durante scene o animazioni lunghe): solo con il pulsante o il tasto qui sopra.");

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Strascico del dash dopo un atterraggio");
			ImGuiMCP::TextWrapped("Se atterri a meta' dash il personaggio puo' restare a velocita' folle per 10-15 secondi (bug 'supersonico'). Solo nei 25 secondi dopo un dash, e solo se la velocita' e' oltre il doppio di quella delle schivate e delle mosse speciali, la mod la limita. Schivate e mosse di combattimento non vengono mai toccate. Ogni episodio finisce nel log con tutti i dettagli.");
			Check("Limita lo strascico del dash", s.speedFix, "Spento = la mod non tocca mai la velocita' a terra dopo un dash.");
			if (!s.speedFix) {
				ImGuiMCP::BeginDisabled();
			}
			SliderF("Velocita' massima durante lo strascico (m/s)", s.speedFixMax, 6.0f, 40.0f, "%.1f", 12.0f,
				"Vale solo mentre lo strascico dura. Una corsa normale e' ~5 m/s, lo sprint ~7.");
			Check("Blocca lo spostamento eccessivo", s.speedFixPin,
				"Durante lo strascico il personaggio non puo' spostarsi piu' del limite: se il motore lo sposta troppo lo riporta indietro (dal primo frame dopo l'atterraggio).");
			Check("Cerca la velocita' alla radice", s.speedFixSource,
				"Durante uno strascico cerca nel processo del personaggio il valore di velocita' 'nascosto' da cui il motore ricava Speed e la velocita' delle gambe, e lo riporta al limite. Agisce solo se lo riconosce con sicurezza (lo scrive nel log). Non scrive mai dati di Havok.");
			Check("Limita anche le variabili di velocita' del grafo", s.speedFixVars,
				"Durante lo strascico riporta al limite anche le altre variabili del grafo animazioni che hanno 'speed' nel nome (moltiplicatori e simili esclusi), cosi' le gambe non girano alla velocita' della luce.");
			Check("Se non basta, ripristina le animazioni", s.speedFixReset,
				"Dopo 2 secondi di strascico fa lo stesso ripristino del pulsante di reset. Spento di default: non ha mai aiutato.");
			if (!s.speedFix) {
				ImGuiMCP::EndDisabled();
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Movimenti attivi");
			ImGuiMCP::TextWrapped("Spegni solo quello che non vuoi: gli altri restano uguali. Vale subito.");
			Check("Doppio salto", s.doubleJumpEnabled, "Spento = niente salti in aria (resta solo il salto normale). Richiede il gate dei salti configurato.");
			if (!s.doubleJumpEnabled) {
				ImGuiMCP::BeginDisabled();
			}
			Check("Triplo salto", s.tripleJumpEnabled, "Spento = al massimo il doppio salto.");
			if (!s.doubleJumpEnabled) {
				ImGuiMCP::EndDisabled();
			}
			Check("Super salto", s.superJumpEnabled);
			Check("Dash in aria", s.dashEnabled);
			Check("Slam", s.slamEnabled);

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Multi-salto");
			if (!st.bjFileFound) {
				ImGuiMCP::TextColored(kYellow, "BetterJumpingSE.txt non trovato.");
				ImGuiMCP::TextWrapped("Il numero di salti e l'altezza dei salti successivi li gestisce il gate dei salti.");
			} else if (st.bjMatch) {
				ImGuiMCP::TextColored(kGreen, "Configurazione corretta (perk %08X).", st.bjPerkInFile);
			} else {
				ImGuiMCP::TextColored(kRed, "Il gate dei salti usa il perk %08X, serve %08X.", st.bjPerkInFile, st.bjPerkExpected);
				ImGuiMCP::TextWrapped("Senza correzione il multi-salto non parte. Premi il pulsante e poi riavvia il gioco.");
				if (st.bjFixedNow) {
					ImGuiMCP::TextColored(kGreen, "Fatto! Ora riavvia Skyrim.");
				} else if (ImGuiMCP::Button("Correggi gate dei salti")) {
					if (!Core::FixJumpGate()) {
						RE::SendHUDMessage::ShowHUDMessage("MJN: non riesco a scrivere BetterJumpingSE.txt (vedi il log).");
					}
				}
			}
		}

		void __stdcall PageJumps()
		{
			Core::MenuTouched();
			auto& s = Settings::Get();
			ImGuiMCP::SeparatorText("Salto e gravita'");
			SliderF("Altezza del salto", s.jumpHeight, 40.0f, 250.0f, "%.1f", 86.25f, "Skyrim normale = 76.");
			SliderF("Gravita' in caduta", s.gravity, 0.5f, 3.0f, "%.2f", 1.35f, "Piu' basso = cadi piano, piu' alto = cadi veloce. Il valore del tuo gioco e' scritto qui sotto.");
			if (const float vg = Core::VanillaGravity(); vg > 0.0f) {
				ImGuiMCP::Text("Gravita' del tuo Skyrim: %.2f (Predefinito qui sopra usa 1.35)", vg);
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Movimento in aria");
			SliderF("Movimento in aria (%)", s.airControl, 0.0f, 200.0f, "%.0f %%", 100.0f,
				"100 = invariato (la mod non interviene). Sotto 100 ti sposti meno tra un salto e l'altro, sopra 100 di piu'. 0 = solo su e giu'.");
			ImGuiMCP::TextWrapped("Funzione sperimentale: se non senti differenze o qualcosa si comporta male, rimetti 100.");

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Salti in aria");
			Check("Doppio salto", s.doubleJumpEnabled, "Spento = niente salti in aria (resta solo il salto normale).");
			if (!s.doubleJumpEnabled) {
				ImGuiMCP::BeginDisabled();
			}
			Check("Triplo salto", s.tripleJumpEnabled, "Spento = al massimo il doppio salto.");
			if (!s.doubleJumpEnabled) {
				ImGuiMCP::EndDisabled();
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Animazioni e atterraggio");
			Check("Animazioni di atterraggio", s.animatedLanding, "Usa le animazioni di atterraggio della mod dopo le cadute alte.");
			Check("Cambio direzione tra un salto e l'altro", s.directionChange, "Se in aria cambi direzione con lo stick prima del salto successivo, il personaggio cambia davvero direzione di movimento (e parte l'animazione dedicata).");
			if (!s.directionChange) {
				ImGuiMCP::BeginDisabled();
			}
			SliderF("Peso del cambio direzione (%)", s.directionWeight, 0.0f, 100.0f, "%.0f %%", 0.0f,
				"0 = leggero: cambi direzione di colpo, come prima. Piu' sale, piu' il cambio e' pesante: il personaggio curva piano e conserva piu' a lungo lo slancio della direzione di prima (100 = circa un secondo per una virata di 180 gradi).");
			SliderF("Velocita' minima da fermo (m/s)", s.directionMinSpeed, 1.0f, 10.0f, "%.1f", 4.0f,
				"Se al salto eri quasi fermo, il cambio direzione ti da questa velocita'. Se eri gia' in movimento si tiene la velocita' che avevi. La velocita' finale dipende anche da 'Movimento in aria'.");
			Check("Ruota il personaggio nella nuova direzione", s.directionTurnBody,
				"Il modello del personaggio gira insieme alla direzione di movimento. In prima persona non fa nulla (girerebbe la visuale).");
			if (!s.directionChange) {
				ImGuiMCP::EndDisabled();
			}
			Check("Niente danni da caduta", s.noFallDamage, "Aggiunge il perk 'atterraggio morbido' (danno da caduta a zero).");

			ImGuiMCP::Spacing();
			ImGuiMCP::TextWrapped("Il numero massimo di salti resta quello di BetterJumpingSE.txt (3); qui sopra puoi solo ridurlo.");
		}

		void __stdcall PageSuperJump()
		{
			Core::MenuTouched();
			auto& s = Settings::Get();
			ImGuiMCP::SeparatorText("Super salto");
			Check("Super salto attivo", s.superJumpEnabled);
			SliderF("Altezza super salto", s.superJumpHeight, 150.0f, 1500.0f, "%.0f", 750.0f);
			SliderF("Tempo di pressione (secondi)", s.superJumpHold, 0.3f, 4.0f, "%.2f", 1.5f, "Per quanto tieni premuto il tasto prima che il super salto sia pronto.");
			ImGuiMCP::Spacing();
			KeyPicker("Tasto (tieni premuto)##sj", g_kSuper, g_iSuper, s.keySuperJump, "Tieni premuto fino al messaggio 'Super salto pronto', poi salta.");
			PadPicker(0, "padsj", s.padSuperJump, 0x0004, "Tieni premuto sul controller fino al messaggio 'Super salto pronto', poi salta.");
			WarnSameKey();
			ImGuiMCP::TextWrapped("Dopo il messaggio hai 5 secondi per saltare: il salto successivo e' un super salto.");
		}

		void __stdcall PageDash()
		{
			Core::MenuTouched();
			auto& s = Settings::Get();
			ImGuiMCP::SeparatorText("Dash in aria");
			Check("Dash attivo", s.dashEnabled);

			int dist = s.dashDistance;
			if (ImGuiMCP::SliderInt("Distanza", &dist, 1, 3, "%d")) {
				s.dashDistance = std::clamp(dist, 1, 3);
				s.MarkDirty();
			}
			Tip("1 = corto, 2 = medio, 3 = lungo. Sono le tre animazioni di dash disponibili.");
			ImGuiMCP::SameLine();
			const char* names[] = { "", "corto", "medio", "lungo" };
			ImGuiMCP::Text("%s", names[std::clamp(s.dashDistance, 1, 3)]);

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Dopo il dash");
			Check("Limita la velocita' dopo il dash", s.dashSpeedLimit,
				"Dopo un dash Skyrim a volte lascia al personaggio la velocita' del movimento dell'animazione e questo parte a velocita' enorme. Con questo acceso, per alcuni secondi dopo il dash (e all'atterraggio) la velocita' orizzontale non supera il limite qui sotto.");
			if (!s.dashSpeedLimit) {
				ImGuiMCP::BeginDisabled();
			}
			SliderF("Velocita' massima dopo il dash (m/s)", s.dashExitMaxSpeed, 4.0f, 20.0f, "%.1f", 9.0f,
				"9 m/s e' poco sopra lo sprint normale. Se ti sembra che dopo il dash tu venga frenato troppo, alzalo.");
			if (!s.dashSpeedLimit) {
				ImGuiMCP::EndDisabled();
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Aspetto e suono");
			if (ImGuiMCP::RadioButton("Effetto di fuoco (cerchio rosso + scintille)", &s.dashStyle, 0)) {
				s.MarkDirty();
			}
			if (ImGuiMCP::RadioButton("Effetto alternativo (cerchio e scia)", &s.dashStyle, 1)) {
				s.MarkDirty();
			}
			Tip("Vale dal dash successivo. Ricorda: l'effetto si spegne del tutto dalla pagina Effetti.");
			Check("Suono del dash", s.dashSound, "Spento = dash silenzioso.");
			Check("Suoni in ordine casuale", s.dashSoundRandom, "Spento = i 5 suoni girano uno dopo l'altro. Acceso = a caso, mai lo stesso due volte di fila.");

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Comandi");
			KeyPicker("Tasto (premi in aria)##dash", g_kDash, g_iDash, s.keyDash, "Premi il tasto mentre sei in aria.");
			PadPicker(1, "paddash", s.padDash, 0x0008, "Premi il pulsante (o la combinazione) mentre sei in aria.");
			WarnSameKey();
			ImGuiMCP::TextWrapped("Il dash parte sempre in aria, anche dopo il primo salto normale.");
		}

		void __stdcall PageSlam()
		{
			Core::MenuTouched();
			auto& s = Settings::Get();
			ImGuiMCP::SeparatorText("Slam a terra");
			Check("Slam attivo", s.slamEnabled);
			SliderF("Tempo di pressione (secondi)", s.slamHold, 0.0f, 1.5f, "%.2f", 0.25f, "Quanto devi tenere premuto prima che lo slam parta (evita le pressioni involontarie).");
			Check("Movimento libero durante la carica", s.slamFreeMovement, "Mentre carichi lo slam puoi muoverti, invece di restare bloccato dall'animazione.");
			ImGuiMCP::Spacing();
			KeyPicker("Tasto (tieni premuto in aria)##slam", g_kSlam, g_iSlam, s.keySlam, "Tieni premuto in aria: rallentatore, poi precipiti. Tieni premuto fino a terra.");
			PadPicker(2, "padslam", s.padSlam, 0x0108, "Tieni premuto in aria: rallentatore, poi precipiti. Tieni premuto fino a terra.");
			WarnSameKey();
			ImGuiMCP::TextWrapped("Tieni premuto in aria: dopo un attimo al rallentatore precipiti a terra. Rilascia per annullare.");

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Effetto dello slam");
			Check("Tempesta di Fuoco all'atterraggio", s.slamFireStorm,
				"All'atterraggio parte la magia Tempesta di Fuoco (la stessa della pergamena), al posto dell'onda d'urto. Non costa magicka. Il filtro 'Slam' della pagina Effetti riguarda solo l'anello dell'onda d'urto.");
			if (!s.slamFireStorm) {
				ImGuiMCP::BeginDisabled();
			}
			SliderF("Attesa tra due Tempeste di Fuoco (secondi)", s.fireStormCooldown, 0.0f, 900.0f, "%.0f s", 300.0f, "300 secondi = 5 minuti. 0 = nessuna attesa.");
			Check("Durante l'attesa usa l'onda d'urto normale", s.slamShockwaveWhileWaiting,
				"Acceso: se la Tempesta di Fuoco non e' pronta lo slam fa l'onda d'urto di prima. Spento: durante l'attesa lo slam non ha nessun effetto all'atterraggio.");
			if (!Core::FireStormFound()) {
				ImGuiMCP::TextColored(kRed, "Magia Tempesta di Fuoco non trovata (vedi il log): lo slam usa l'onda d'urto.");
			} else {
				const double left = Core::FireStormRemaining();
				if (left <= 0.0) {
					ImGuiMCP::TextColored(kGreen, "Tempesta di Fuoco: pronta.");
				} else {
					const int sec = static_cast<int>(std::ceil(left));
					ImGuiMCP::TextColored(kYellow, "Tempesta di Fuoco: pronta tra %d:%02d", sec / 60, sec % 60);
					ImGuiMCP::SameLine();
					if (ImGuiMCP::Button("Azzera attesa")) {
						Core::FireStormResetCooldown();
					}
				}
			}
			if (!s.slamFireStorm) {
				ImGuiMCP::EndDisabled();
			}
		}

		// confermare prima di toglierli: il pulsante lavora sul salvataggio in uso
		bool g_legacyConfirm = false;

		void __stdcall PageAirAttack()
		{
			Core::MenuTouched();
			auto& s = Settings::Get();
			ImGuiMCP::SeparatorText("Attacchi in salto");
			ImGuiMCP::TextWrapped(
				"Le animazioni degli attacchi sono in OAR. Qui ci sono i comandi e la logica: "
				"senza le animazioni i comandi funzionano ma non si vede l'animazione. La magia in salto avvia la strega volante (pagina dedicata).");
			Check("Attacchi in salto attivi", s.airAttackEnabled, "Spento = in aria non succede nulla con questi comandi.");
			if (!s.airAttackEnabled) {
				ImGuiMCP::BeginDisabled();
			}
			Check("In qualunque salto", s.airAttackAlways,
				"Acceso = puoi attaccare in aria in ogni salto e in ogni caduta. Spento = solo dopo un salto fatto con la mod.");
			Check("Scelta automatica con l'attacco normale", s.airAttackAutoPick,
				"Con un arco in mano l'attacco normale scocca una freccia, con sole magie in mano avvia la strega volante, altrimenti colpisce con l'arma.");

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Comandi (in aria)");
			ImGuiMCP::TextColored(kYellow, "Attacco normale");
			KeyPicker("Tasto##airn", g_kAirN, g_iAirN, s.keyAirNormal, "Premi in aria: colpo con l'arma in mano.");
			PadPicker(4, "padairn", s.padAirNormal, 0x20000, "Predefinito RT (il pulsante di attacco destro).");
			ImGuiMCP::TextColored(kYellow, "Attacco potente");
			KeyPicker("Tasto##airp", g_kAirP, g_iAirP, s.keyAirPower, "Premi in aria: attacco potente.");
			PadPicker(5, "padairp", s.padAirPower, 0x30000, "Predefinito LT + RT. Se il comando contiene l'attacco normale, tieni LT e premi RT.");
			ImGuiMCP::TextColored(kYellow, "Magia (strega volante)");
			KeyPicker("Tasto##airc", g_kAirC, g_iAirC, s.keyAirCast, "Premi in aria con una magia in mano: parte la modalita' strega volante (pagina Strega volante).");
			PadPicker(6, "padairc", s.padAirCast, 0x10000, "Predefinito LT (il pulsante di attacco sinistro).");
			ImGuiMCP::TextColored(kYellow, "Arco / balestra");
			KeyPicker("Tasto##airr", g_kAirR, g_iAirR, s.keyAirRanged, "Premi in aria: scocca una freccia (serve un arco o una balestra in mano e almeno una freccia).");
			PadPicker(7, "padairr", s.padAirRanged, 0, "Nessuno di default: con l'arco basta l'attacco normale.");
			WarnAirAttackClash();
			ImGuiMCP::TextWrapped("Con le impostazioni di default: RT = attacco normale, LT + RT = attacco potente, LT = strega volante. Se premi LT e RT quasi insieme conta come LT + RT.");

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Attacco potente in salto");
			SliderF("Altezza minima (unita')##airminh", s.airPowerMinHeight, 0.0f, 400.0f, "%.0f", 115.0f,
				"Altezza sopra il punto di partenza del salto sotto la quale l'attacco potente non parte. 115 = poco sopra un salto normale (circa l'altezza del secondo salto). 0 = sempre. "
				"Se sei caduto da un bordo conta l'altezza dal terreno.");
			Check("Burst di fuoco con le armi a due mani", s.heavyBurst,
				"L'attacco potente in salto con spada, ascia o martello a due mani fa partire una piccola esplosione di fuoco attorno a te: e' la stessa della Tempesta di Fuoco dello slam, ma con area e danno ridotti. Senza attesa tra una e l'altra.");
			if (s.heavyBurst) {
				ImGuiMCP::Indent();
				SliderF("Danno da fuoco##burstdmg", s.heavyBurstDamage, 0.0f, 200.0f, "%.0f", 25.0f, "Danno dell'esplosione a ogni nemico nell'area (la Tempesta di Fuoco dello slam viene riportata a questo valore).");
				SliderF("Raggio (piedi)##burstrad", s.heavyBurstRadius, 3.0f, 30.0f, "%.0f", 10.0f, "Area dell'esplosione: 10 piedi = circa 3 metri. Lo slam usa l'area intera della Tempesta di Fuoco, questa e' molto piu' piccola.");
				SliderF("Velocita' degli effetti (x)##burstspeed", s.heavyBurstSpeed, 1.0f, 3.0f, "%.2f", 1.5f,
					"Quanto piu' veloce si muove l'animazione degli effetti della Tempesta di Fuoco del burst (1 = come nel gioco, 1.5 = una volta e mezzo piu' veloce). La dimensione non cambia. Vale dal prossimo burst.");
				ImGuiMCP::Unindent();
			}
			if (!s.airAttackEnabled) {
				ImGuiMCP::EndDisabled();
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Stato");
			ImGuiMCP::TextWrapped("%s", Core::AirAttackStatus().c_str());
		}

		void __stdcall PageWitch()
		{
			Core::MenuTouched();
			auto& s = Settings::Get();
			ImGuiMCP::SeparatorText("Strega volante");
			ImGuiMCP::TextWrapped(
				"In aria, con una magia in mano, il tasto della magia in salto (pagina Attacchi in salto) ti lascia sospeso nel punto in cui sei, senza scendere. "
				"Lanci le magie normalmente, con le animazioni dedicate (singola, doppia, concentrazione, protezione, telecinesi, rituale...). "
				"Salto, dash e slam sono fermi. Si esce col tasto qui sotto: il personaggio comincia a cadere solo quando l'animazione di unequip e' finita. "
				"Questa modalita' sostituisce l'attacco con la magia in salto.");
			Check("Strega volante attiva", s.witchEnabled, "Spenta = in aria il tasto della magia non fa niente.");
			if (!s.witchEnabled) {
				ImGuiMCP::BeginDisabled();
			}
			ImGuiMCP::Spacing();
			SliderF("Velocita' di movimento (m/s)##witchspeed", s.witchSpeed, 0.5f, 8.0f, "%.1f", 2.5f,
				"Velocita' massima mentre sei sospeso, in tutte le direzioni orizzontali (stick o tasti di movimento). Una camminata e' circa 1.9.");
			{
				// durata massima: 30 s ... 299 s, in fondo "Illimitato" (nel file vale 0)
				float t = (s.witchTimer >= 30.0f && s.witchTimer < 300.0f) ? s.witchTimer : 300.0f;
				if (SliderF("Durata massima (secondi)##witchtimer", t, 30.0f, 300.0f, t >= 299.5f ? "Illimitato" : "%.0f s", 300.0f,
						"Dopo questo tempo la modalita' finisce da sola (con l'animazione di unequip), 10 secondi prima compare un avviso. Tutto a destra = illimitata (come prima). Il minimo e' 30 secondi.")) {
					s.witchTimer = t >= 299.5f ? 0.0f : std::round(t);
				}
			}
			SliderF("Fluttuazione su e giu' (unita')##witchbob", s.witchBob, 0.0f, 20.0f, "%.0f", 5.0f,
				"Quanto il modello sale e scende piano mentre e' sospeso, per far sembrare che galleggi. 0 = fermo.");

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Uscita");
			KeyPicker("Tasto di uscita##wx1", g_kWitch1, g_iWitch1, s.keyWitchExit, "Premilo per finire la modalita'. Predefinito: freccia sinistra.");
			KeyPicker("Secondo tasto (facoltativo)##wx2", g_kWitch2, g_iWitch2, s.keyWitchExit2, "Se lo imposti servono tutti e due i tasti premuti insieme.");
			PadPicker(8, "padwitchexit", s.padWitchExit, 0x0104, "Predefinito LB + D-pad sinistra. Puoi assegnare anche una combinazione di pulsanti.");
			WarnAirAttackClash();
			Check("Rinfoderare le magie esce dalla modalita'", s.witchExitOnSheathe, "Il comando del gioco per riporre le mani finisce la modalita' (con l'animazione di unequip).");

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Animazioni ed effetti");
			Check("Mostra l'equip anche a mani gia' fuori", s.witchReequip,
				"Se le mani magiche sono gia' fuori quando premi, le rimette via e le rifa' uscire per mostrare l'animazione di equip (richiede un paio di secondi in piu'). Spento = parte subito.");
			Check("Effetti visivi", s.witchFx, "Aura sul corpo e sulle mani per tutta la durata della modalita'; si spengono quando esci.");
			Check("Il gioco non riceve movimento (niente camminata in aria)", s.witchLegsLock,
				"Mentre sei sospeso stick e tasti di movimento vengono fermati prima che arrivino al gioco: il gioco non vede nessun movimento, quindi il personaggio non usa le animazioni di camminata e resta nella posa delle magie. "
				"Ti sposta solo il plugin, leggendo lo stick per conto suo. Le altre azioni (magie, uscita, camera) funzionano normalmente.");
			Check("Via la posa di caduta all'attivazione", s.witchGroundPose,
				"All'attivazione manda al personaggio gli eventi di atterraggio del gioco, uno alla volta e solo finche' serve, per uscire dalla posa di caduta. La posa delle mani non viene toccata.");
			Check("Posa di caduta quando esci", s.witchFallEvent,
				"All'uscita fa ripartire l'animazione di caduta e da' una piccola spinta verso il basso, cosi' il personaggio precipita invece di scendere piano.");
			Check("Blocca tutto tranne lancio e uscita", s.witchBlockActions,
				"Mentre sei sospeso sono bloccati salto, sprint, furtivita', urlo e schivata (anche quella delle altre mod): il comando non arriva al gioco e l'evento di animazione corrispondente viene fermato prima che arrivi al personaggio. "
				"Restano i lanci delle magie, il comando di uscita e il movimento fatto dal plugin. Il log elenca gli eventi fermati a fine modalita'.");
			Check("Blocca la posa di bacino e gambe", s.witchBoneLock,
				"Se durante i lanci (dual casting compreso) la posa del personaggio scende verso quella da terra, bacino e gambe vengono tenuti nella posa sospesa dell'ultimo momento buono, mentre braccia e busto continuano a fare l'animazione di lancio. "
				"E' un blocco forzato: se noti deformazioni strane del corpo spegnilo e mandami il log.");
			if (!s.witchEnabled) {
				ImGuiMCP::EndDisabled();
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Stato");
			ImGuiMCP::TextWrapped("%s", Core::WitchStatus().c_str());
		}

		void __stdcall PageGlide()
		{
			Core::MenuTouched();
			auto& s = Settings::Get();
			ImGuiMCP::SeparatorText("Mod di planata");
			ImGuiMCP::TextWrapped(
				"Con la mod accesa le animazioni di caduta della mod accendono bAnimationDriven: durante la planata il movimento viene guidato "
				"da un'animazione che non si muove e il personaggio resta quasi fermo. Questa modalita' riconosce la planata e sistema le cose finche' dura.");
			ImGuiMCP::TextWrapped("Il file MagicalJumpingNative.log racconta cosa succede (righe che iniziano con \"planata:\").");
			ImGuiMCP::Spacing();
			Check("Compatibilita' planata", s.paraCompat, "Spenta = la mod non fa niente di speciale in planata (utile per confrontare nel log).");
			if (s.paraCompat) {
				ImGuiMCP::Indent();
				Check("Tieni spento bAnimationDriven e i flag di caduta", s.paraClearDriven,
					"Mentre plani spegne a ogni frame bAnimationDriven e i flag di caduta che le animazioni della mod accendono.");
				Check("Metti in pausa la mod (dash, slam, super salto, cambio direzione)", s.paraPause,
					"Mentre plani i comandi della mod non fanno niente e non tocca la velocita' del personaggio.");
				Check("Gravita' di Skyrim", s.paraVanillaGravity, "Mentre plani usa la gravita' del gioco invece di quella impostata qui.");
				Check("Spegni le animazioni della mod mentre plani", s.paraSuspend,
					"Spegne l'interruttore delle animazioni (E91): OAR smette di sostituire le animazioni, come a mod spenta. E' una sola scrittura e viene rimessa a fine planata "
					"(se la planata si ripete subito dopo resta spenta, senza riaccendere e spegnere ogni volta).");
				if (s.paraSuspend) {
					ImGuiMCP::Indent();
					Check("Togli anche i perk (gate dei salti e animazioni)", s.paraRemovePerks,
						"Piu' invasivo: aggiunge e toglie perk al personaggio. Di solito non serve, perche' le animazioni dipendono gia' dall'interruttore. Usalo solo se la planata va ancora male.");
					ImGuiMCP::Unindent();
				}
				ImGuiMCP::Unindent();
			}
			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Stato");
			ImGuiMCP::TextWrapped("%s", Core::GlideStatus().c_str());
		}

		void __stdcall PageLegacy()
		{
			Core::MenuTouched();
			ImGuiMCP::SeparatorText("Disattiva i vecchi script");
			ImGuiMCP::TextWrapped(
				"Serve per togliere il vecchio plugin delle impostazioni (e i vecchi script) senza che il gioco vada in crash al caricamento. "
				"Quel plugin lascia nel salvataggio dei riferimenti e dei controller che, se il plugin sparisce, mandano in crash Skyrim.");
			ImGuiMCP::Spacing();
			ImGuiMCP::TextWrapped("Il pulsante, sul personaggio e sul salvataggio in uso:");
			ImGuiMCP::BulletText("ferma le quest dei vecchi script;");
			ImGuiMCP::BulletText("toglie gli effetti magici attivi e gli incantesimi dei vecchi script;");
			ImGuiMCP::BulletText("elimina i riferimenti che gli script hanno creato nel mondo (controller e stack del plugin delle impostazioni, volumi del dash).");
			ImGuiMCP::TextWrapped("Non tocca i file dei tuoi mod e non toglie nulla da MO2: quello lo fai tu dopo.");

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Procedura");
			ImGuiMCP::TextWrapped("1. Premi il pulsante qui sotto (due passaggi, per sicurezza) e aspetta il messaggio verde.");
			ImGuiMCP::TextWrapped("2. Salva il gioco in un NUOVO salvataggio (non sovrascrivere l'unico che hai).");
			ImGuiMCP::TextWrapped("3. Chiudi Skyrim e togli il vecchio plugin delle impostazioni (plugin e script) dalla lista dei mod.");
			ImGuiMCP::TextWrapped("4. Rientra, carica il salvataggio e ripulisci quello che resta con ResaverSSE.");
			ImGuiMCP::TextColored(kYellow, "Se usi il vecchio anello, toglilo prima di premere.");

			ImGuiMCP::Spacing();
			const bool busy = Core::LegacyBusy();
			if (busy) {
				ImGuiMCP::TextColored(kYellow, "Disattivazione in corso, qualche secondo...");
				g_legacyConfirm = false;
			} else if (!g_legacyConfirm) {
				if (ImGuiMCP::Button("Disattiva i vecchi script...")) {
					g_legacyConfirm = true;
				}
				Tip("Non cambia nulla finche' non confermi al passaggio successivo.");
			} else {
				ImGuiMCP::TextColored(kRed, "Sicuro? Lo fai sul salvataggio in uso: fermo le quest, tolgo gli effetti e cancello i riferimenti dei vecchi script.");
				if (ImGuiMCP::Button("Si', disattiva ora")) {
					g_legacyConfirm = false;
					Core::RequestLegacyCleanup();
				}
				ImGuiMCP::SameLine();
				if (ImGuiMCP::Button("Annulla")) {
					g_legacyConfirm = false;
				}
			}

			const std::string res = Core::LegacyStatus();
			if (!res.empty()) {
				ImGuiMCP::Spacing();
				ImGuiMCP::SeparatorText("Esito");
				ImGuiMCP::TextColored(res.rfind("Fatto", 0) == 0 ? kGreen : kYellow, "%s", res.c_str());
				ImGuiMCP::TextWrapped("Il dettaglio di quello che e' stato fermato o eliminato e' nel file MagicalJumpingNative.log.");
			}
		}

		void __stdcall PageEffects()
		{
			Core::MenuTouched();
			auto& s = Settings::Get();
			ImGuiMCP::SeparatorText("Effetti visivi per movimento");
			ImGuiMCP::TextWrapped("Togli luci, bagliori e particelle di un movimento senza toccare il resto: animazione, altezza e gameplay restano uguali. Vale dal salto successivo.");
			ImGuiMCP::Spacing();
			Check("Primo salto", s.fxJump1, "Effetti del primo salto (quello che apre il doppio salto).");
			Check("Doppio salto", s.fxJump2);
			Check("Triplo salto", s.fxJump3);
			Check("Dash", s.fxDash, "Cerchio e scintille mentre scatti (non cambia il suono: quello e' nella pagina Dash).");
			Check("Slam", s.fxSlam, "Spegne l'anello d'onda con la distorsione dello schermo (onda d'urto dello slam). Lo stordimento dei nemici vicini resta. Non cambia la Tempesta di Fuoco.");
			Check("Atterraggio", s.fxLanding, "Polvere e bagliore quando atterri dopo una caduta alta.");
			if (s.fxLanding) {
				ImGuiMCP::Indent();
				Check("Bagliore rosso", s.fxLandingRed, "Il bagliore della protezione dalla caduta e' rosso invece che blu. Vale dal prossimo atterraggio.");
				if (s.fxLandingRed) {
					ImGuiMCP::Indent();
					Check("Togli anche le scintille del gioco", s.fxLandingNoSparks, "Da accendere solo se dopo il bagliore rosso restano scintille blu: toglie l'effetto di scintille del gioco.");
					ImGuiMCP::Unindent();
				}
				ImGuiMCP::Unindent();
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::SeparatorText("Fiamme (solo grafica, nessun danno)");
			ImGuiMCP::TextWrapped("Scia di fuoco, rune ed esplosione fatte con le risorse di Skyrim: non fanno danno e non usano script.");
			ImGuiMCP::Text("Aspetto della scia di fuoco (super salto e attacco pesante):");
			if (ImGuiMCP::RadioButton("Solo fiamme sul corpo##trail0", &s.fxTrailStyle, 0)) {
				s.MarkDirty();
			}
			if (ImGuiMCP::RadioButton("Fiamme + arte di fiamme intorno al corpo (come nella 2.6)##trail1", &s.fxTrailStyle, 1)) {
				s.MarkDirty();
			}
			if (ImGuiMCP::RadioButton("Solo arte di fiamme intorno al corpo##trail2", &s.fxTrailStyle, 2)) {
				s.MarkDirty();
			}
			Tip("Vale dalla prossima volta che parte la scia. Se vedi ancora il 'turbine' di fuoco intorno al personaggio prova un altro stile e dimmi quale e' quello giusto.");
			Check("Super salto", s.fxEdSuperJump,
				"Quando salti con il super salto attivo parte la scia di fuoco, che dura finche' non atterri (si spegne anche quando parte la planata), e per due secondi compaiono le rune intorno al personaggio (quelle della strega volante). Mentre il super salto e' pronto (prima del salto) c'e' solo il bagliore rosso. Con un salto normale non succede niente.");
			Check("Esplosione del super salto da terra", s.fxSjExplosion,
				"Quando fai il super salto da terra parte una piccola esplosione sotto i piedi, solo grafica e suono (nessun danno, nessuna spinta): l'idea e' che salti grazie all'esplosione. Non parte per i salti in aria.");
			if (s.fxSjExplosion) {
				ImGuiMCP::Indent();
				SliderF("Dimensione (x)##sjexplsize", s.fxSjExplosionSize, 0.1f, 1.5f, "%.2f", 0.45f,
					"Dimensione dell'esplosione rispetto a quella intera (1 = intera). 0.45 = poco piu' grande del personaggio. Se vedi l'esplosione sempre alla dimensione intera, la riga 'esplosione:' del log dice se il gioco ha permesso di cambiarla.");
				ImGuiMCP::Unindent();
			}
			Check("Attacco pesante in salto", s.fxEdHeavy,
				"All'inizio dell'attacco pesante in aria parte solo la scia di fuoco, che dura finche' non atterri; all'atterraggio c'e' l'esplosione a terra (solo visiva). Il burst di fuoco col danno (armi a due mani) resta quello di prima.");
			Check("Salvataggio dalla caduta", s.fxEdFallSave,
				"Le rune intorno al personaggio (quelle della strega volante, per due secondi) quando parte l'animazione di salvataggio dalla caduta, insieme al bagliore rosso e con le sue stesse condizioni. Niente fiamme. Vale dal prossimo atterraggio.");

			ImGuiMCP::Spacing();
			if (ImGuiMCP::Button("Tutti accesi")) {
				s.fxJump1 = s.fxJump2 = s.fxJump3 = s.fxDash = s.fxSlam = s.fxLanding = true;
				s.fxEdSuperJump = s.fxEdHeavy = s.fxEdFallSave = s.fxSjExplosion = true;
				s.MarkDirty();
			}
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button("Tutti spenti")) {
				s.fxJump1 = s.fxJump2 = s.fxJump3 = s.fxDash = s.fxSlam = s.fxLanding = false;
				s.fxEdSuperJump = s.fxEdHeavy = s.fxEdFallSave = s.fxSjExplosion = false;
				s.MarkDirty();
			}
		}
	}

	void Register()
	{
		if (!GetMenuFrameworkModule()) {
			logger::warn("SKSE Menu Framework non trovato: niente menu F1 (le impostazioni restano nel file .ini)");
			return;
		}
		SKSEMenuFramework::SetSection("MJN");
		SKSEMenuFramework::AddSectionItem("Generale", PageGeneral);
		SKSEMenuFramework::AddSectionItem("Salti e gravita'", PageJumps);
		SKSEMenuFramework::AddSectionItem("Super salto", PageSuperJump);
		SKSEMenuFramework::AddSectionItem("Dash", PageDash);
		SKSEMenuFramework::AddSectionItem("Slam", PageSlam);
		SKSEMenuFramework::AddSectionItem("Attacchi in salto", PageAirAttack);
		SKSEMenuFramework::AddSectionItem("Strega volante", PageWitch);
		SKSEMenuFramework::AddSectionItem("Effetti", PageEffects);
		SKSEMenuFramework::AddSectionItem("Planata", PageGlide);
		SKSEMenuFramework::AddSectionItem("Disattiva vecchi script", PageLegacy);
		logger::info("menu F1 registrato");
	}
}
