<script>
export let connection;
let code = ''

function join() {
    connection.joinGame(code);
}
</script>

<main>
<h3>Join a game</h3>
<input placeholder="Room code" bind:value={code}>
<button on:click={join}>Join</button>
</main>

<style>
input {
    text-transform: uppercase;
}
input::placeholder {
    text-transform: none;
}
</style>