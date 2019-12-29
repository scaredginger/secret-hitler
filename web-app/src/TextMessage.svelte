<script>
export let ondismiss = () => {};
</script>

<main>
<button on:click={ondismiss}>
  Dismiss
</button>
<slot name="title">
</slot>
<slot name="info">
</slot>
<slot name="text">
</slot>
</main>

<style>
</style>
