# MJN 2.3 — progetto PERSONALE, non una release

> **ATTENZIONE: questo e' un progetto per uso personale.**
> E' un **mashup di funzioni di varie mod che mi piacciono**, messo insieme per il *mio* gioco e il *mio* elenco di mod.
> **Non e' una mod rilasciata per tutti**: non e' pensato per essere scaricato, installato o usato da altre persone,
> non offro supporto e non accetto richieste.
> Il repository e' pubblico **solo** perche' un repository privato mi ha limitato l'uso di GitHub Actions per compilare.

## Che cos'e' (e che cosa NON e')

- E' un plugin SKSE **senza Papyrus**, scritto per me, che raccoglie in un unico posto alcune funzioni di movimento
  e di combattimento in aria che mi piacciono, prese da varie mod che uso e riscritte a modo mio. **Mashup per uso personale.**
- **Non e' una release pubblica.** Non c'e' una pagina della mod, non ci sono versioni da scaricare, non c'e' un pacchetto pronto.
- Le idee, le animazioni, gli effetti e i suoni da cui parte questo mashup sono **del lavoro dei rispettivi autori** e
  **non sono in questo repository** (qui ci sono solo il codice del plugin e il file per compilarlo).
- Se stai leggendo questo file e non sei io: questo repository e' un archivio di lavoro personale, non un prodotto.
  **Non e' pensato per altri.**

## Cosa c'e' in questo repository

| File | Cosa e' |
|---|---|
| `src/` | il codice sorgente del plugin (C++), per uso personale |
| `xmake.lua` | il file di compilazione |
| `.github/workflows/build.yml` | il workflow che compila il plugin su GitHub Actions (build personale) |
| `README.md` | queste note, per me |

Non contiene animazioni, mesh, texture, suoni, plugin `.esp` o altri dati di gioco: quelli stanno solo sul mio computer.

## Cosa fa la versione 2.3 (sempre: per il mio uso)

La 1.0 e' la base di movimento (gia' provata da me). La 2.x aggiunge:

1. **Strega volante** (modalita' di levitazione con le magie)
   - In aria, con le magie in mano, il tasto di lancio in salto ti lascia **sospeso nel punto in cui sei** (niente discesa graduale).
   - All'attivazione le magie che hai in mano **escono da sole** (animazione di equip): la richiesta parte appena il personaggio
     esce dalla posa di salto / caduta e viene ripetuta (ogni 0,7 s, al massimo 6 volte) finche' le mani non sono fuori. All'uscita
     parte l'animazione di unequip; durante la modalita' le animazioni di lancio cambiano in base al tipo di magia
     (lancio singolo, doppio, concentrazione, ecc.).
   - Ci si muove **lentamente in tutte le direzioni** mentre si lanciano magie (velocita' regolabile dal menu).
   - Non funzionano salto, dash, slam, super salto, attacchi in aria: solo il lancio delle magie e il comando di uscita.
   - Il comando di uscita e' configurabile (tastiera: anche **due tasti insieme**; controller: anche combinazioni).
   - Quando esco, il personaggio **comincia a cadere solo quando finisce l'animazione di unequip**; in quel momento riparte
     l'animazione di caduta e il personaggio riceve una piccola spinta verso il basso (non resta "appoggiato" all'aria).
   - Mentre e' sospeso resta **fermo nella posa delle magie**, senza animazioni di camminata, e si sposta fluttuando: il gioco non
     riceve l'input di movimento (lo stick lo leggo io e ti muovo io con la velocita' del controller). Le animazioni di lancio che
     hanno movimento delle gambe si vedono normalmente.
   - All'attivazione scende piano (di serie 90 unita', regolabile fino a 250; compensa la telecamera in terza persona) e poi **fluttua
     leggermente su e giu'** (spostamento del modello, non un'animazione).
   - **Durata massima** regolabile dal menu: da 30 secondi a illimitata (10 secondi prima della fine compare un avviso).
   - Per tutta la durata si accendono gli effetti visivi (corpo e mani), che si spengono quando esco.
   - Sostituisce, **solo per le magie**, l'attacco in aria con le magie della 1.0. Il resto resta invariato.
2. **Attacchi pesanti in salto con armi a due mani**: all'impatto parte una **piccola esplosione di fuoco**, la stessa della
   Tempesta di Fuoco dello slam a terra ma con area e danno molto ridotti (regolabili), **senza attesa** tra una e l'altra.
3. **Altezza minima per gli attacchi pesanti in aria**: funzionano solo piu' in alto di un salto normale
   (circa l'altezza del secondo salto, regolabile). Troppo in basso, non partono.
4. **Bagliore rosso** (al posto del blu) nell'effetto di protezione dalla caduta da grande altezza.

Il resto (salto regolabile, doppio/triplo salto, super salto, dash in aria, slam a terra, cambio direzione in aria,
attacchi in salto normali/potenti/con l'arco, correzioni di sicurezza, reset delle animazioni) e' quello della 1.0.

Tutto si imposta dal menu **F1 -> MJN** (richiede SKSE Menu Framework). Le impostazioni sono salvate in
`Data/SKSE/Plugins/MagicalJumpingNative.ini` (ogni voce e' commentata nel file). Il log e' in
`Documents/My Games/Skyrim Special Edition/SKSE/MagicalJumpingNative.log`.

## Tasti predefiniti (tutti cambiabili dal menu)

| Azione | Tastiera | Controller | Modo |
|---|---|---|---|
| Super salto | freccia Su | D-pad Sinistra | tieni premuto (1,5 s), poi salta |
| Dash in aria | freccia Destra | D-pad Destra | pressione singola, solo in aria |
| Slam | freccia Giu' | LB + D-pad Destra | tieni premuto in aria (0,25 s) |
| Uscita dalla strega volante | freccia Sinistra (+ un secondo tasto opzionale) | LB + D-pad Sinistra | pressione singola |
| Reset animazioni | nessuno | LB + RB + Back | tieni premuto (0,6 s) |
| Attacco in aria: normale / potente / magia | tasto del gioco | RT / LT + RT / LT | come gli attacchi a terra |

I pulsanti del controller si leggono direttamente da XInput; nel menu "Cambia" ti fa premere il pulsante o la combinazione.

## Strega volante: note di funzionamento

- Parte dal comando di lancio in salto (o dall'attacco normale, con l'opzione "Scelta automatica con l'attacco normale") quando in mano hai
  **solo magie** (con l'arco parte la freccia, con le armi parte l'attacco in aria).
- Richiede gli attacchi in aria attivi nel menu e i dati di gioco della 2.0 (che non sono in questo repository).
- Se le mani magiche sono gia' fuori all'attivazione, vengono rinfoderate e rimesse per mostrare l'animazione di equip
  (opzione "Mostra l'equip anche a mani gia' fuori"); costa circa 2 secondi in piu'.
- Se le mani sono riposte, escono da sole appena il grafo di animazione lascia lo stato di salto (circa 1 secondo, poi l'animazione
  di equip). Se dopo 6 secondi non sono uscite la modalita' parte lo stesso (riga `mani non uscite` nel log).
- Rinfoderare le magie col comando del gioco fa uscire dalla modalita' (opzione).
- Il movimento e' solo orizzontale (8 direzioni); la quota la tiene il plugin (abbassamento iniziale + fluttuazione).
- Tre opzioni nel menu controllano come si vede il personaggio: "Gambe ferme" (mentre sei sospeso l'input di movimento viene tolto
  PRIMA dell'update del giocatore e, in piu', durante l'aggiornamento delle animazioni vengono azzerati velocita' del controller e
  variabili di velocita': niente camminata), "Via la posa di caduta all'attivazione" (eventi di atterraggio del gioco, uno alla
  volta, `JumpLand` per primo, senza toccare la posa delle mani) e "Posa di caduta quando esci".
- Il salto premuto mentre sei sospeso non conta come salto di questo volo.
- Il log (`strega volante: ...`) scrive i primi tre episodi in dettaglio: quota, velocita' lette e scritte, stato del controller e
  del grafo di animazione, altezze di testa / bacino / telecamera rispetto al punto del personaggio, richieste di "mani fuori" e,
  quando ti muovi, una riga `movimento:` con l'input vero e le variabili del grafo (Speed, Direction, bInMoveState, ...);
  dopo l'uscita scrive anche cosa succede per due secondi.

## Compilare (GitHub Actions)

Il plugin si compila solo su GitHub Actions (non ho una toolchain locale):

1. Carica il contenuto di questa cartella in un repository (compresa `.github/workflows/build.yml`).
2. Il workflow parte con il push (o da *Actions -> Build -> Run workflow*). Una build richiede circa 30 minuti la prima volta.
3. A fine build scarica l'artifact `MJN-2.3-uso-personale`: contiene `SKSE/Plugins/MagicalJumpingNative.dll` (+ `.pdb`).

Base: CommonLibSSE-NG (fork con supporto VR, commit fissato nel workflow), compilato per Skyrim SE 1.5.97 e AE.

## Installazione (per me, in MO2)

1. Mod "dati" (privata, non nel repository): contiene le animazioni OAR, gli effetti, i suoni e il plugin `doublejump.esp` (ESL).
2. Mod "DLL": l'artifact scaricato da GitHub.
3. Dipendenze: SKSE, Address Library, OAR, SKSE Menu Framework.
4. Se arrivo da una versione con script Papyrus: menu F1 -> "Disattiva vecchi script", salva, esci e togli i vecchi file.

## Limiti noti / da verificare in gioco (2.3)

- **Strega volante**: il lancio nativo a mezz'aria e l'estrazione delle mani magiche via codice non sono confermati su tutte le
  versioni del gioco; l'uscita comincia con l'animazione di unequip e la caduta riparte solo alla fine (tempo massimo di sicurezza: 3 s).
- Gli effetti della strega volante si spengono **all'inizio** dell'uscita, non alla fine dell'animazione di unequip.
- L'effetto di impatto del bagliore rosso usa anche un effetto del gioco che potrebbe restare blu: c'e' l'opzione
  "Togli anche le scintille del gioco".
- L'esplosione di fuoco copia gli effetti della Tempesta di Fuoco del gioco: l'area e il danno si riducono, ma la grandezza del solo effetto
  visivo potrebbe non seguire l'area. Da verificare a occhio.
- Le cause dei difetti della strega volante (posa di caduta, camminata in aria, discesa lenta all'uscita) sono dedotte, non confermate
  dal log: se restano, le righe del log che iniziano con "strega volante:" dicono lo stato reale.
- "Gambe ferme" toglie l'input di movimento prima dell'update del giocatore: che basti a fermare la camminata e' una deduzione dal
  log (Speed = 68 x velocityMod, calcolati a monte). La riga `movimento:` del log dice se ha funzionato (velocityMod e Speed a 0 con
  input vero diverso da 0).
- L'altezza della posa delle magie (il modello e' piu' in alto di una posa normale) dipende dall'animazione: l'abbassamento sposta il
  personaggio, e se la telecamera segue il punto del personaggio si abbassa anche lei. Le altezze nel log servono a misurarlo.
- Il comando di uscita predefinito del controller (LB + D-pad Sinistra) blocca il super salto con quella combinazione.
- Le impostazioni della sezione di compatibilita' con la planata (`[Glide]` nell'ini) sono ripartite dai valori predefiniti.

## Nomi tecnici che compaiono nel codice

Alcuni nomi (il nome del file `doublejump.esp`, nomi di variabili ed eventi di animazione, nome del file dell'ini e del log)
servono al gioco per riconoscere i file e **non indicano una release pubblica**: sono semplici identificatori tecnici di un
progetto personale. Il file `src/SKSEMenuFramework.h` e' un'intestazione di terze parti, dei suoi autori.

## Ancora una volta

Questo repository e' **solo mio**, per **uso personale**: un **mashup di varie mod che mi piacciono**.
**Non e' una release per tutti.** Grazie agli autori di tutte le mod da cui ho preso ispirazione: il merito delle idee,
delle animazioni e degli effetti e' loro.
