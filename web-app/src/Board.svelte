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
  <p id="status">
    {$statusText}
  </p>
  <p id="gameKey">
    {$gameKey}
  </p>
  <p id="fascistPolicies">
    FASCIST: { $fascistPolicies }
  </p>
  <p id="liberalPolicies">
    LIBERAL: { $liberalPolicies }
  </p>
  <p id="tracker">
    Election Tracker: { $electionTracker }
  </p>
</main>

<style>
  p {
    font-size: 2rem;
  }
  #fascistPolicies {
    color: red;
  }
  #liberalPolicies {
    color: skyblue;
  }
</style>