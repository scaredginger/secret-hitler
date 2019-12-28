<script>
export let connection;
let code = ''

function create() {
    connection.create(code);
}
</script>

<main>
<h3>Create a game</h3>
<input placeholder="Room code" bind:value={code}>
<button on:click={create}>Join</button>
</main>