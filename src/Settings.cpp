#include "Settings.h"

namespace MJ
{
	namespace
	{
		constexpr auto kIniPath = "Data\\SKSE\\Plugins\\MagicalJumpingNative.ini";

		std::string Trim(const std::string& a_in)
		{
			const auto b = a_in.find_first_not_of(" \t\r\n");
			if (b == std::string::npos) {
				return {};
			}
			const auto e = a_in.find_last_not_of(" \t\r\n");
			return a_in.substr(b, e - b + 1);
		}

		std::string Lower(std::string a_in)
		{
			std::transform(a_in.begin(), a_in.end(), a_in.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return a_in;
		}

		using Map = std::map<std::string, std::string>;

		Map ReadIni()
		{
			Map          out;
			std::ifstream in(kIniPath);
			if (!in.is_open()) {
				return out;
			}
			std::string section;
			std::string line;
			while (std::getline(in, line)) {
				line = Trim(line);
				if (line.empty() || line[0] == ';' || line[0] == '#') {
					continue;
				}
				if (line.front() == '[' && line.back() == ']') {
					section = Lower(Trim(line.substr(1, line.size() - 2)));
					continue;
				}
				const auto eq = line.find('=');
				if (eq == std::string::npos) {
					continue;
				}
				out[section + "." + Lower(Trim(line.substr(0, eq)))] = Trim(line.substr(eq + 1));
			}
			return out;
		}

		bool GetBool(const Map& m, const char* sec, const char* key, bool def)
		{
			const auto it = m.find(Lower(sec) + "." + Lower(key));
			if (it == m.end()) {
				return def;
			}
			const auto v = Lower(it->second);
			if (v == "1" || v == "true" || v == "yes" || v == "on") {
				return true;
			}
			if (v == "0" || v == "false" || v == "no" || v == "off") {
				return false;
			}
			return def;
		}

		float GetFloat(const Map& m, const char* sec, const char* key, float def, float lo, float hi)
		{
			const auto it = m.find(Lower(sec) + "." + Lower(key));
			if (it == m.end()) {
				return def;
			}
			char*       end = nullptr;
			const float v = std::strtof(it->second.c_str(), &end);
			if (end == it->second.c_str() || !std::isfinite(v)) {
				return def;
			}
			return std::clamp(v, lo, hi);
		}

		int GetInt(const Map& m, const char* sec, const char* key, int def, int lo, int hi)
		{
			const auto it = m.find(Lower(sec) + "." + Lower(key));
			if (it == m.end()) {
				return def;
			}
			char*      end = nullptr;
			const long v = std::strtol(it->second.c_str(), &end, 0);
			if (end == it->second.c_str()) {
				return def;
			}
			return static_cast<int>(std::clamp<long>(v, lo, hi));
		}
	}

	double Now()
	{
		using clock = std::chrono::steady_clock;
		return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
	}

	Settings& Settings::Get()
	{
		static Settings instance;
		return instance;
	}

	void Settings::MarkDirty()
	{
		dirty = true;
		dirtyAt = Now();
	}

	void Settings::Load()
	{
		const auto m = ReadIni();
		if (m.empty()) {
			return;
		}
		enabled = GetBool(m, "General", "Enabled", enabled);
		messages = GetBool(m, "General", "Messages", messages);
		padEnabled = GetBool(m, "General", "Gamepad", padEnabled);

		jumpHeight = GetFloat(m, "Jump", "JumpHeight", jumpHeight, 20.0f, 400.0f);
		gravity = GetFloat(m, "Jump", "Gravity", gravity, 0.2f, 5.0f);
		animatedLanding = GetBool(m, "Jump", "AnimatedLanding", animatedLanding);
		directionChange = GetBool(m, "Jump", "DirectionChange", directionChange);
		directionMinSpeed = GetFloat(m, "Jump", "DirectionMinSpeed", directionMinSpeed, 0.5f, 10.0f);
		directionWeight = GetFloat(m, "Jump", "DirectionWeight", directionWeight, 0.0f, 100.0f);
		directionTurnBody = GetBool(m, "Jump", "DirectionTurnBody", directionTurnBody);
		noFallDamage = GetBool(m, "Jump", "NoFallDamage", noFallDamage);
		doubleJumpEnabled = GetBool(m, "Jump", "DoubleJump", doubleJumpEnabled);
		tripleJumpEnabled = GetBool(m, "Jump", "TripleJump", tripleJumpEnabled);
		airControl = GetFloat(m, "Jump", "AirControl", airControl, 0.0f, 200.0f);

		superJumpEnabled = GetBool(m, "SuperJump", "Enabled", superJumpEnabled);
		superJumpHeight = GetFloat(m, "SuperJump", "Height", superJumpHeight, 100.0f, 2000.0f);
		superJumpHold = GetFloat(m, "SuperJump", "HoldSeconds", superJumpHold, 0.2f, 5.0f);
		keySuperJump = GetInt(m, "SuperJump", "Key", keySuperJump, 0, 255);
		padSuperJump = GetInt(m, "SuperJump", "PadButtons", padSuperJump, 0, 0x3F3FF);

		dashEnabled = GetBool(m, "Dash", "Enabled", dashEnabled);
		dashDistance = GetInt(m, "Dash", "Distance", dashDistance, 1, 3);
		keyDash = GetInt(m, "Dash", "Key", keyDash, 0, 255);
		padDash = GetInt(m, "Dash", "PadButtons", padDash, 0, 0x3F3FF);
		dashStyle = GetInt(m, "Dash", "Style", dashStyle, 0, 1);
		dashSpeedLimit = GetBool(m, "Dash", "SpeedLimit", dashSpeedLimit);
		dashExitMaxSpeed = GetFloat(m, "Dash", "ExitMaxSpeed", dashExitMaxSpeed, 3.0f, 30.0f);
		speedFix = GetBool(m, "Safety", "LandingFix", speedFix);
		speedFixMax = GetFloat(m, "Safety", "LandingFixMax", speedFixMax, 6.0f, 60.0f);
		speedFixTrigger = GetFloat(m, "Safety", "LandingFixTrigger", speedFixTrigger, 1500.0f, 10000.0f);
		speedFixPin = GetBool(m, "Safety", "LandingFixPin", speedFixPin);
		speedFixReset = GetBool(m, "Safety", "LandingFixReset", speedFixReset);
		speedFixDeepLog = GetBool(m, "Safety", "LandingFixDeepLog", speedFixDeepLog);
		speedFixSource = GetBool(m, "Safety", "LandingFixSource", speedFixSource);
		speedFixVars = GetBool(m, "Safety", "LandingFixVars", speedFixVars);
		airAttackEnabled = GetBool(m, "AirAttack", "Enabled", airAttackEnabled);
		airAttackAlways = GetBool(m, "AirAttack", "Always", airAttackAlways);
		airAttackAutoPick = GetBool(m, "AirAttack", "AutoPick", airAttackAutoPick);
		keyAirNormal = GetInt(m, "AirAttack", "NormalKey", keyAirNormal, 0, 255);
		padAirNormal = GetInt(m, "AirAttack", "NormalPadButtons", padAirNormal, 0, 0x3F3FF);
		keyAirPower = GetInt(m, "AirAttack", "PowerKey", keyAirPower, 0, 255);
		padAirPower = GetInt(m, "AirAttack", "PowerPadButtons", padAirPower, 0, 0x3F3FF);
		keyAirCast = GetInt(m, "AirAttack", "CastKey", keyAirCast, 0, 255);
		padAirCast = GetInt(m, "AirAttack", "CastPadButtons", padAirCast, 0, 0x3F3FF);
		keyAirRanged = GetInt(m, "AirAttack", "RangedKey", keyAirRanged, 0, 255);
		padAirRanged = GetInt(m, "AirAttack", "RangedPadButtons", padAirRanged, 0, 0x3F3FF);
		airPowerMinHeight = GetFloat(m, "HeavyAttack", "MinHeight", airPowerMinHeight, 0.0f, 600.0f);
		heavyBurst = GetBool(m, "HeavyAttack", "Burst", heavyBurst);
		heavyBurstDamage = GetFloat(m, "HeavyAttack", "BurstDamage", heavyBurstDamage, 0.0f, 500.0f);
		heavyBurstRadius = GetFloat(m, "HeavyAttack", "BurstRadius", heavyBurstRadius, 2.0f, 60.0f);
		heavyBurstSpeed = GetFloat(m, "HeavyAttack", "BurstSpeed", heavyBurstSpeed, 1.0f, 3.0f);

		witchEnabled = GetBool(m, "Witch", "Enabled", witchEnabled);
		witchSpeed = GetFloat(m, "Witch", "Speed", witchSpeed, 0.2f, 12.0f);
		keyWitchExit = GetInt(m, "Witch", "ExitKey", keyWitchExit, 0, 255);
		keyWitchExit2 = GetInt(m, "Witch", "ExitKey2", keyWitchExit2, 0, 255);
		padWitchExit = GetInt(m, "Witch", "ExitPadButtons", padWitchExit, 0, 0x3F3FF);
		witchFx = GetBool(m, "Witch", "Effects", witchFx);
		witchReequip = GetBool(m, "Witch", "ReEquipHands", witchReequip);
		witchExitOnSheathe = GetBool(m, "Witch", "ExitOnSheathe", witchExitOnSheathe);
		witchTimer = GetFloat(m, "Witch", "Timer", witchTimer, 0.0f, 3600.0f);
		if (witchTimer > 0.0f && witchTimer < 30.0f) {
			witchTimer = 30.0f;
		}
		witchBob = GetFloat(m, "Witch", "Bob", witchBob, 0.0f, 40.0f);
		witchLegsLock = GetBool(m, "Witch", "LegsLock", witchLegsLock);
		witchGroundPose = GetBool(m, "Witch", "GroundPose", witchGroundPose);
		witchFallEvent = GetBool(m, "Witch", "FallEvent", witchFallEvent);
		witchBlockActions = GetBool(m, "Witch", "BlockActions", witchBlockActions);
		witchBoneLock = GetBool(m, "Witch", "BoneLock", witchBoneLock);
		witchHideLanding = GetBool(m, "Witch", "HideLanding", witchHideLanding);
		witchNoGlide = GetBool(m, "Witch", "NoGlideAfterExit", witchNoGlide);
		witchSmooth = GetBool(m, "Witch", "SmoothUpperBody", witchSmooth);
		dashSound = GetBool(m, "Dash", "Sound", dashSound);
		dashSoundRandom = GetBool(m, "Dash", "SoundRandom", dashSoundRandom);

		paraCompat = GetBool(m, "Glide", "Compat", paraCompat);
		paraClearDriven = GetBool(m, "Glide", "ClearAnimationDriven", paraClearDriven);
		paraPause = GetBool(m, "Glide", "PauseMod", paraPause);
		paraVanillaGravity = GetBool(m, "Glide", "SkyrimGravity", paraVanillaGravity);
		paraSuspend = GetBool(m, "Glide", "SuspendAnimations", paraSuspend);
		paraRemovePerks = GetBool(m, "Glide", "RemovePerks", paraRemovePerks);

		fxJump1 = GetBool(m, "Effects", "Jump", fxJump1);
		fxJump2 = GetBool(m, "Effects", "DoubleJump", fxJump2);
		fxJump3 = GetBool(m, "Effects", "TripleJump", fxJump3);
		fxDash = GetBool(m, "Effects", "Dash", fxDash);
		fxSlam = GetBool(m, "Effects", "Slam", fxSlam);
		fxLanding = GetBool(m, "Effects", "Landing", fxLanding);
		fxLandingRed = GetBool(m, "Effects", "LandingRed", fxLandingRed);
		fxLandingNoSparks = GetBool(m, "Effects", "LandingNoSparks", fxLandingNoSparks);
		fxEdSuperJump = GetBool(m, "Effects", "FlamesSuperJump", fxEdSuperJump);
		fxEdHeavy = GetBool(m, "Effects", "FlamesHeavyAttack", fxEdHeavy);
		fxEdFallSave = GetBool(m, "Effects", "FlamesFallSave", fxEdFallSave);
		fxSjExplosion = GetBool(m, "Effects", "ExplosionSuperJump", fxSjExplosion);
		fxSjExplosionSize = GetFloat(m, "Effects", "ExplosionSuperJumpSize", fxSjExplosionSize, 0.1f, 1.5f);
		fxTrailStyle = GetInt(m, "Effects", "TrailStyle", fxTrailStyle, 0, 2);

		keyReset = GetInt(m, "Reset", "Key", keyReset, 0, 255);
		padReset = GetInt(m, "Reset", "PadButtons", padReset, 0, 0x3F3FF);
		resetHold = GetFloat(m, "Reset", "HoldSeconds", resetHold, 0.2f, 3.0f);

		slamEnabled = GetBool(m, "Slam", "Enabled", slamEnabled);
		slamHold = GetFloat(m, "Slam", "HoldSeconds", slamHold, 0.0f, 3.0f);
		slamFreeMovement = GetBool(m, "Slam", "FreeMovement", slamFreeMovement);
		keySlam = GetInt(m, "Slam", "Key", keySlam, 0, 255);
		padSlam = GetInt(m, "Slam", "PadButtons", padSlam, 0, 0x3F3FF);
		slamFireStorm = GetBool(m, "Slam", "FireStorm", slamFireStorm);
		fireStormCooldown = GetFloat(m, "Slam", "FireStormCooldown", fireStormCooldown, 0.0f, 3600.0f);
		slamShockwaveWhileWaiting = GetBool(m, "Slam", "ShockwaveWhileWaiting", slamShockwaveWhileWaiting);
		fireStormFormId = GetInt(m, "Slam", "FireStormFormID", fireStormFormId, 0, 0x7FFFFFFF);
	}

	void Settings::Save()
	{
		dirty = false;
		std::ofstream out(kIniPath, std::ios::trunc);
		if (!out.is_open()) {
			logger::warn("impossibile salvare {}", kIniPath);
			return;
		}
		auto b = [](bool v) { return v ? 1 : 0; };
		out << "; MJN - impostazioni (si modificano dal menu F1)\n";
		out << "; Key = tasto della tastiera (codice Windows VK: 38=freccia su, 39=destra, 40=giu', 37=sinistra; 0 = nessuno).\n";
		out << "; PadButtons = pulsanti del controller (somma di: Su=1 Giu'=2 Sinistra=4 Destra=8 Start=16 Back=32 L3=64 R3=128\n";
		out << ";   LB=256 RB=512 A=4096 B=8192 X=16384 Y=32768 LT=65536 RT=131072; 0 = nessuno). Meglio cambiarli dal menu F1.\n\n";
		out << "[General]\n";
		out << "Enabled=" << b(enabled) << "\n";
		out << "Messages=" << b(messages) << "\n";
		out << "Gamepad=" << b(padEnabled) << "\n\n";
		out << "[Jump]\n";
		out << "JumpHeight=" << std::format("{:.2f}", jumpHeight) << "\n";
		out << "Gravity=" << std::format("{:.2f}", gravity) << "\n";
		out << "AnimatedLanding=" << b(animatedLanding) << "\n";
		out << "DirectionChange=" << b(directionChange) << "\n";
		out << "; DirectionMinSpeed: velocita' (m/s) che il cambio direzione in aria da' al personaggio se era quasi fermo\n";
		out << "DirectionMinSpeed=" << std::format("{:.1f}", directionMinSpeed) << "\n";
		out << "; DirectionWeight: peso del cambio direzione. 0 = istantaneo, 100 = curva lenta (circa 1 s per una virata di 180 gradi)\n";
		out << "DirectionWeight=" << std::format("{:.0f}", directionWeight) << "\n";
		out << "; DirectionTurnBody: 1 = il modello del personaggio gira nella nuova direzione (solo terza persona)\n";
		out << "DirectionTurnBody=" << b(directionTurnBody) << "\n";
		out << "NoFallDamage=" << b(noFallDamage) << "\n";
		out << "; DoubleJump/TripleJump: 0 = quel salto in aria e' disattivato (TripleJump=0 -> al massimo il doppio salto)\n";
		out << "DoubleJump=" << b(doubleJumpEnabled) << "\n";
		out << "TripleJump=" << b(tripleJumpEnabled) << "\n";
		out << "; AirControl: % del movimento orizzontale in aria. 100 = invariato\n";
		out << "AirControl=" << std::format("{:.0f}", airControl) << "\n\n";
		out << "[SuperJump]\n";
		out << "Enabled=" << b(superJumpEnabled) << "\n";
		out << "Height=" << std::format("{:.0f}", superJumpHeight) << "\n";
		out << "HoldSeconds=" << std::format("{:.2f}", superJumpHold) << "\n";
		out << "Key=" << keySuperJump << "\n";
		out << "PadButtons=" << padSuperJump << "\n\n";
		out << "[Dash]\n";
		out << "Enabled=" << b(dashEnabled) << "\n";
		out << "Distance=" << dashDistance << "\n";
		out << "Key=" << keyDash << "\n";
		out << "PadButtons=" << padDash << "\n";
		out << "; Style: 0 = fuoco (cerchio e scintille di fuoco), 1 = effetto alternativo\n";
		out << "Style=" << dashStyle << "\n";
		out << "; SpeedLimit: 1 = dopo il dash la velocita' orizzontale non supera ExitMaxSpeed (m/s). Evita lo scatto improvviso a velocita' enorme\n";
		out << "SpeedLimit=" << b(dashSpeedLimit) << "\n";
		out << "ExitMaxSpeed=" << std::format("{:.1f}", dashExitMaxSpeed) << "\n";
		out << "Sound=" << b(dashSound) << "\n";
		out << "SoundRandom=" << b(dashSoundRandom) << "\n\n";
		out << "[Glide]\n";
		out << "; Compat: 1 = quando il personaggio plana (mod di planata) la mod si adatta. Gli interruttori sotto valgono solo con Compat=1\n";
		out << "Compat=" << b(paraCompat) << "\n";
		out << "; ClearAnimationDriven: durante la planata tiene spento bAnimationDriven (acceso dalle animazioni di caduta di MJN: blocca il movimento)\n";
		out << "ClearAnimationDriven=" << b(paraClearDriven) << "\n";
		out << "; PauseMod: durante la planata niente dash, slam, super salto, cambio direzione, movimento in aria e limitatore di velocita'\n";
		out << "PauseMod=" << b(paraPause) << "\n";
		out << "; SkyrimGravity: durante la planata usa la gravita' originale di Skyrim\n";
		out << "SkyrimGravity=" << b(paraVanillaGravity) << "\n";
		out << "; SuspendAnimations: durante la planata spegne l'interruttore delle animazioni (tutte le regole OAR della mod lo richiedono): e' una sola scrittura, rimessa alla fine\n";
		out << "SuspendAnimations=" << b(paraSuspend) << "\n";
		out << "; RemovePerks: in piu' toglie i perk del gate dei salti e delle animazioni durante la planata (e li rimette). Di solito non serve\n";
		out << "RemovePerks=" << b(paraRemovePerks) << "\n\n";
		out << "[Effects]\n";
		out << "Jump=" << b(fxJump1) << "\n";
		out << "DoubleJump=" << b(fxJump2) << "\n";
		out << "TripleJump=" << b(fxJump3) << "\n";
		out << "Dash=" << b(fxDash) << "\n";
		out << "Slam=" << b(fxSlam) << "\n";
		out << "Landing=" << b(fxLanding) << "\n";
		out << "; LandingRed: 1 = il bagliore di protezione dalla caduta e' rosso (0 = blu come prima)\n";
		out << "LandingRed=" << b(fxLandingRed) << "\n";
		out << "; LandingNoSparks: con il rosso acceso, 1 = toglie anche le scintille del gioco (se dopo il bagliore rosso ne restano di blu)\n";
		out << "LandingNoSparks=" << b(fxLandingNoSparks) << "\n";
		out << "; Effetti solo visivi (nessun danno): FlamesSuperJump = al super salto (scia di fuoco fino all'atterraggio, rune per 2 s, bagliore rosso mentre e' pronto); FlamesHeavyAttack = attacco pesante in salto (scia di fuoco e esplosione a terra); FlamesFallSave = salvataggio dalla caduta (rune intorno al personaggio, insieme al bagliore rosso)\n";
		out << "FlamesSuperJump=" << b(fxEdSuperJump) << "\n";
		out << "FlamesHeavyAttack=" << b(fxEdHeavy) << "\n";
		out << "FlamesFallSave=" << b(fxEdFallSave) << "\n";
		out << "; ExplosionSuperJump: 1 = il super salto da terra parte con una piccola esplosione solo visiva (effetto e suono, nessun danno). ExplosionSuperJumpSize: dimensione rispetto all'esplosione intera (0.1-1.5; 0.45 = poco piu' del personaggio)\n";
		out << "ExplosionSuperJump=" << b(fxSjExplosion) << "\n";
		out << "ExplosionSuperJumpSize=" << std::format("{:.2f}", fxSjExplosionSize) << "\n";
		out << "; TrailStyle: aspetto della scia di fuoco (super salto, attacco pesante): 0 = solo fiamme sul corpo, 1 = fiamme + arte di fiamme intorno al corpo (come nella 2.6), 2 = solo arte di fiamme\n";
		out << "TrailStyle=" << fxTrailStyle << "\n\n";
		out << "[Reset]\n";
		out << "; reset animazioni del personaggio (tieni premuto HoldSeconds). Key 0 / PadButtons 0 = nessuno. Default pad: LB+RB+Back = 800\n";
		out << "Key=" << keyReset << "\n";
		out << "PadButtons=" << padReset << "\n";
		out << "HoldSeconds=" << std::format("{:.2f}", resetHold) << "\n\n";
		out << "[Safety]\n";
		out << "; Il reset automatico delle animazioni non esiste piu': si fa solo con il tasto / il pulsante del menu.\n";
		out << "; LandingFix: 1 = dopo un atterraggio a meta' dash il personaggio puo' restare a velocita' folle per 10-15 s. Se nei 25 s dopo un dash la variabile Speed del grafo supera LandingFixTrigger, la mod limita il movimento a LandingFixMax m/s.\n";
		out << "; Non tocca mai schivate e mosse speciali di altre mod (Speed fino a ~1500) e non fa niente in dash, slam, planata, a cavallo, in acqua.\n";
		out << "LandingFix=" << b(speedFix) << "\n";
		out << "LandingFixMax=" << std::format("{:.1f}", speedFixMax) << "\n";
		out << "; LandingFixTrigger: valore di Speed oltre il quale scatta (1 m/s = Speed 68; 3000 = 44 m/s)\n";
		out << "LandingFixTrigger=" << std::format("{:.0f}", speedFixTrigger) << "\n";
		out << "; LandingFixPin: 1 = durante lo strascico il personaggio non puo' spostarsi piu' di LandingFixMax: se il motore lo sposta troppo lo riporta indietro, dal primo frame dopo l'atterraggio\n";
		out << "LandingFixPin=" << b(speedFixPin) << "\n";
		out << "; LandingFixReset: 1 = se dopo 2 secondi e' ancora attivo fa il ripristino delle animazioni (spento di default)\n";
		out << "LandingFixReset=" << b(speedFixReset) << "\n";
		out << "; LandingFixDeepLog: 1 = nel log di un episodio scrive anche gli effetti attivi che toccano SpeedMult, le variabili del grafo e i campi del controller. Mettilo a 0 se il gioco si chiude proprio li' (l'ultimo passo del crash log lo dice)\n";
		out << "LandingFixDeepLog=" << b(speedFixDeepLog) << "\n";
		out << "; LandingFixSource: 1 = durante lo strascico cerca nel processo del personaggio la velocita' 'nascosta' da cui il motore ricava Speed e velocityMod e la riporta al limite (solo se si riconosce con sicurezza)\n";
		out << "LandingFixSource=" << b(speedFixSource) << "\n";
		out << "; LandingFixVars: 1 = durante lo strascico limita anche le altre variabili di velocita' del grafo (nome che contiene 'speed', tranne moltiplicatori e simili)\n";
		out << "LandingFixVars=" << b(speedFixVars) << "\n\n";
		out << "[AirAttack]\n";
		out << "; Attacchi in salto. Le animazioni sono in OAR: senza i comandi funzionano ma non si vede nulla.\n";
		out << "; Always: 1 = in qualunque salto; 0 = solo nella finestra di un salto magico. AutoPick: 1 = con l'arco l'attacco normale scocca una freccia, con sole magie in mano avvia la strega volante\n";
		out << "Enabled=" << b(airAttackEnabled) << "\n";
		out << "Always=" << b(airAttackAlways) << "\n";
		out << "AutoPick=" << b(airAttackAutoPick) << "\n";
		out << "NormalKey=" << keyAirNormal << "\n";
		out << "NormalPadButtons=" << padAirNormal << "\n";
		out << "PowerKey=" << keyAirPower << "\n";
		out << "PowerPadButtons=" << padAirPower << "\n";
		out << "CastKey=" << keyAirCast << "\n";
		out << "CastPadButtons=" << padAirCast << "\n";
		out << "RangedKey=" << keyAirRanged << "\n";
		out << "RangedPadButtons=" << padAirRanged << "\n\n";
		out << "[HeavyAttack]\n";
		out << "; MinHeight: altezza minima (unita' di gioco sopra il punto di partenza del salto) per far partire un attacco pesante in salto. 115 = poco sopra un salto normale; 0 = sempre\n";
		out << "MinHeight=" << std::format("{:.0f}", airPowerMinHeight) << "\n";
		out << "; Burst: 1 = con un'arma a due mani l'attacco pesante in salto fa partire un piccolo burst di fuoco (senza attesa). BurstDamage = danno da fuoco, BurstRadius = raggio in piedi di gioco (10 = circa 3 m)\n";
		out << "Burst=" << b(heavyBurst) << "\n";
		out << "BurstDamage=" << std::format("{:.0f}", heavyBurstDamage) << "\n";
		out << "BurstRadius=" << std::format("{:.0f}", heavyBurstRadius) << "\n";
		out << "; BurstSpeed: velocita' dell'animazione degli effetti del burst (1 = come nel gioco, 1.5 = una volta e mezzo piu' veloce). La dimensione non cambia\n";
		out << "BurstSpeed=" << std::format("{:.2f}", heavyBurstSpeed) << "\n\n";
		out << "[Witch]\n";
		out << "; Modalita' strega volante: in aria, con le magie in mano, il tasto della magia in salto ti lascia sospeso dove sei. Si esce col tasto/pulsanti qui sotto (o rinfoderando le magie).\n";
		out << "Enabled=" << b(witchEnabled) << "\n";
		out << "; Speed: velocita' massima del movimento mentre sei sospeso, in m/s\n";
		out << "Speed=" << std::format("{:.1f}", witchSpeed) << "\n";
		out << "; ExitKey / ExitKey2: tasto per uscire (con ExitKey2 diverso da 0 servono tutti e due premuti). ExitPadButtons: pulsanti del controller (anche combinazioni)\n";
		out << "ExitKey=" << keyWitchExit << "\n";
		out << "ExitKey2=" << keyWitchExit2 << "\n";
		out << "ExitPadButtons=" << padWitchExit << "\n";
		out << "Effects=" << b(witchFx) << "\n";
		out << "; ReEquipHands (2.8, nuovo nome: il vecchio valore ReEquip viene ignorato): se le mani magiche sono gia' fuori quando parte la modalita', le rinfoda e le rimette per mostrare l'animazione di equip\n";
		out << "ReEquipHands=" << b(witchReequip) << "\n";
		out << "ExitOnSheathe=" << b(witchExitOnSheathe) << "\n";
		out << "; Timer: durata massima della modalita' in secondi (minimo 30). 0 = illimitata\n";
		out << "Timer=" << std::format("{:.0f}", witchTimer) << "\n";
		out << "; Bob: ampiezza della fluttuazione su e giu' (unita' di gioco). 0 = ferma\n";
		out << "Bob=" << std::format("{:.1f}", witchBob) << "\n";
		out << "; LegsLock: 1 = il gioco non riceve nessun movimento mentre sei sospeso (ti sposta solo il plugin, le gambe non camminano). GroundPose: 1 = all'attivazione il personaggio esce dalla posa di caduta. FallEvent: 1 = all'uscita riparte la posa di caduta\n";
		out << "LegsLock=" << b(witchLegsLock) << "\n";
		out << "GroundPose=" << b(witchGroundPose) << "\n";
		out << "FallEvent=" << b(witchFallEvent) << "\n";
		out << "; BlockActions: 1 = in modalita' sono bloccati salto, sprint, furtivita', urlo, schivata (anche di altre mod) e ogni evento di animazione di quel tipo; restano i lanci e l'uscita\n";
		out << "BlockActions=" << b(witchBlockActions) << "\n";
		out << "; BoneLock: 1 = se la posa scende verso quella da terra (lanci, dual casting) bacino e gambe vengono tenuti piano piano nella posa sospesa (2.8: correzione continua, non a scatti; non agisce durante l'equip)\n";
		out << "BoneLock=" << b(witchBoneLock) << "\n";
		out << "; HideLanding: 1 = all'attivazione l'atterraggio in aria non si vede (bacino, gambe e busto restano nella posa sospesa, memorizzata e salvata in MagicalJumpingNative.pose; la primissima volta in assoluto l'atterraggio si vede ancora)\n";
		out << "HideLanding=" << b(witchHideLanding) << "\n";
		out << "; NoGlideAfterExit: 1 = dopo l'uscita la mod di planata non parte da sola: cadi con la gravita' normale fino all'atterraggio (3.0)\n";
		out << "NoGlideAfterExit=" << b(witchNoGlide) << "\n";
		out << "; SmoothUpperBody: 1 = i cambi bruschi di posa di busto, spalle e braccia (inizio dei lanci, dual casting) vengono ammorbiditi in circa 0,2 s (3.0)\n";
		out << "SmoothUpperBody=" << b(witchSmooth) << "\n\n";
		out << "[Slam]\n";
		out << "Enabled=" << b(slamEnabled) << "\n";
		out << "HoldSeconds=" << std::format("{:.2f}", slamHold) << "\n";
		out << "FreeMovement=" << b(slamFreeMovement) << "\n";
		out << "Key=" << keySlam << "\n";
		out << "PadButtons=" << padSlam << "\n";
		out << "; FireStorm: 1 = all'atterraggio dello slam parte la magia Tempesta di Fuoco, al massimo una ogni FireStormCooldown secondi (300 = 5 minuti)\n";
		out << "FireStorm=" << b(slamFireStorm) << "\n";
		out << "FireStormCooldown=" << std::format("{:.0f}", fireStormCooldown) << "\n";
		out << "; ShockwaveWhileWaiting: 1 = durante l'attesa lo slam usa l'onda d'urto di prima\n";
		out << "ShockwaveWhileWaiting=" << b(slamShockwaveWhileWaiting) << "\n";
		out << "; FireStormFormID: FormID della magia in Skyrim.esm (0x0007A82B). Cambialo solo se nel log compare 'Tempesta di Fuoco: non trovata'\n";
		out << "FireStormFormID=0x" << std::format("{:08X}", fireStormFormId) << "\n";
	}
}
