<script>
export let options;
export let ondecision;

let displayed = false;

function show() {
  displayed = true;
}

const f = option => () => ondecision(option);
</script>

<main>
<slot name="title">
</slot>
<slot name="info">
</slot>
{#if displayed}
{#each options as option}
  <button on:click={f(option.id)} disabled={!option.eligible}>
    {option.text}
  </button>
{/each}
{/if}

<div id="revealArea" on:mousedown={show} on:touchstart={show}><p>Reveal</p></div>
</main>

<style>
slot {
    position: relative;
}

main {
    height: 100%;
}

#revealArea {
    position: absolute;
    bottom: 0;
    height: 20%;
    width: 100%;
    background: #333;
    user-select: none;
}

#revealArea:active {
    background: #222;
    border: 1px solid #ccc;
    height: calc(20% - 2px);
    width: calc(100% - 2px);
}

#revealArea p {
    vertical-align: middle;
    font-size: 3em;
}

</style>
