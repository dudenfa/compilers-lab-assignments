# Very Busy Expressions - Dataflow Analysis

| Campo | Valore |
| --- | --- |
| Problema | Very Busy Expressions |
| Tipo di analisi | Must analysis |
| Direzione | Backward |
| Dominio | Insiemi di espressioni disponibili nel programma (`E`) |
| Meet operator | Intersezione (`∩`) |
| Valore top | `E` |
| Valore bottom | `∅` |
| Inizializzazione | `OUT[exit] = ∅`, per gli altri nodi tipicamente `OUT[n] = E` |
| Transfer function | `IN[n] = GEN[n] ∪ (OUT[n] - KILL[n])` |
| Equazione di propagazione | `OUT[n] = ⋂ IN[s]`, per ogni successore `s` di `n` |
| Condizione di convergenza | Punto fisso delle equazioni di dataflow |
| Interpretazione di `IN[n]` | Espressioni sicuramente valutate su ogni cammino da `n` a `exit`, prima che i loro operandi vengano modificati |
