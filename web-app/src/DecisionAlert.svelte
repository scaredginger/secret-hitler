<script>
export let options;
export let ondismiss;
</script>

<button on:click={ondismiss}>Dismiss</button>
<main>
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
</style>
