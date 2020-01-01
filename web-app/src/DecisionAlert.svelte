<script>
export let options;
export let ondismiss;
</script>

<main>
<div id="close-button">
<button on:click={ondismiss}>
X
</button>
</div>
<slot name="title">
</slot>
<slot name="info">
</slot>
{#each options as option}
  {#if option.eligible}
    <p>
      {option.text}
    </p>
  {/if}
{/each}
</main>

<style>
#close-button {
    position: relative;
    top: 0;
    right: 0;
    left: 100%;
    transform: translateX(-100%);
    height: 10%;
    width: 100%;
    text-align: right;
    font-size: 2em;
}

slot {
    position: relative;
}

main {
    height: 100%;
}

</style>
