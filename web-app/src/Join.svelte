<script>
import { getContext } from 'svelte';
const client = getContext('client');
let code = '';
let name = '';

function join() {
    if (code)
        client.join(code).then(() => {
            client.setName(name);
        });
    return false;
}
</script>

<main>
<h3>Join a game</h3>
<form action="" on:submit|preventDefault={join}>
    <input placeholder="Code" bind:value={code}>
    <input placeholder="Name" bind:value={name}>
    <input type="submit" value="Join">
</form>
</main>