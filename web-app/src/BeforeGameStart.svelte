<script>
import { getContext } from 'svelte'
import { fascistPolicies, liberalPolicies, gameKey, players, electionTracker, statusText } from './stores';

const client = getContext('client');
let ready = client.isReady;

function readyUp() {
  client.ready();
  ready = true;
}

</script>

<main>
  {#if !ready}
    <button on:click={readyUp}>Ready</button>
  {/if}
  {#if $gameKey}
    <p id="gameKey">
        Game key: {$gameKey}
    </p>
  {/if}
  {#each $players as player}
    <p>{player}</p>
  {/each}
</main>

<style>
  p {
    font-size: 2rem;
  }
</style>