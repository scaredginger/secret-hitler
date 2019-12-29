<script>
export let options;
export let ondecision;

const f = option => () => ondecision(option);
</script>

<main>
<slot name="title">
</slot>
<slot name="info">
</slot>
{#each options as option}
  <button on:click={f(option.id)} disabled={!option.eligible}>
    {option.text}
  </button>
{/each}
</main>

<style>
</style>
