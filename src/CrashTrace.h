#pragma once

// Diagnostica dei crash: non cambia il comportamento del gioco, serve solo a capire DOPO un crash cosa stava facendo il plugin.
//  - Step():  "cosa sto facendo adesso" (un puntatore a una stringa fissa, costo nullo)
//  - Note():  ultimo evento importante (caricamento, salvataggio, cambio di zona...)
//  - Install(): osservatore passivo degli errori gravi di memoria (violazione di accesso ecc.). Quando ne vede passare uno scrive UNA riga in
//    SKSE\MagicalJumpingNative_crash.log con: tipo di errore, in quale DLL e a che offset, se questo plugin compare nello stack,
//    l'ultimo passo e l'ultimo evento. Non cattura, non blocca e non nasconde niente: l'errore prosegue come prima.
namespace MJ::Trace
{
	void        Install();
	void        Step(const char* a_what);
	void        Note(const char* a_what);
	void        Tick();                  // chiamato a ogni frame del plugin
	double      SecondsSinceTick();      // quanto tempo fa e' girato l'ultimo frame del plugin (-1 = mai)
	const char* CurrentStep();
}
