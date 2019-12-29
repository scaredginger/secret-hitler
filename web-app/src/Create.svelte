<script>
import { getContext } from 'svelte';
const client = getContext('client');
let name = '';

function setName() {
  client.create().then(() => {
    client.setName(name);
  });
  return false;
}

</script>

<main>
<h3>Create a game</h3>
<form on:submit|preventDefault={setName}>
    <input placeholder="Name" bind:value={name}>
    <input type="submit" value="Join">
</form>
</main>