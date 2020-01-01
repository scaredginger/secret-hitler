<script>
import { getContext, onMount } from 'svelte';
const client = getContext('client');
let code = '';
let name = '';
let codeInput;

function join() {
    if (code && name)
        client.join(code).then(() => {
            client.setName(name);
        });
    return false;
}

function checkSubmit(e) {
    if (e.key === 'Enter') {
        join();
    }
}

onMount(() => {
    codeInput.focus();
});
</script>

<main>
<h3>Join a game</h3>
<form action="" on:submit|preventDefault={join}>
    <input placeholder="Code" bind:this={codeInput} bind:value={code}>
    <input placeholder="Name" bind:value={name}>
    <input type="submit" value="Join">
</form>
</main>