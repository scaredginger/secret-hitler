<script>
import { getContext, onMount } from 'svelte';
const client = getContext('client');
let name = '';
let nameInput;

function setName() {
  client.create().then(() => {
    client.setName(name);
  });
  return false;
}

onMount(() => {
  nameInput.focus();
});

</script>

<main>
<h3>Create a game</h3>
<form on:submit|preventDefault={setName}>
    <input bind:this={nameInput} placeholder="Name" bind:value={name}>
    <input type="submit" value="Join">
</form>
</main>