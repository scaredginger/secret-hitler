<script>
import Join from './Join.svelte';
export let connection = null;
let mode = '';
function createGame() {
	connection.createGame();
	mode = 'create';
}

function joinGame() {
	mode = 'join';
}

</script>

<main>
	{#if mode === 'join'}
		<Join connection = {connection} />
	{:else}
		<h1>Secret Hitler</h1>
		<p>
			<button on:click={createGame}>Create Game</button>
		</p>

		<p>
			<button on:click={joinGame}>Join Game</button>
		</p>
	{/if}

</main>

<style>
	main {
		text-align: center;
		width: 100%;
		margin: 0 auto;
	}

	h1 {
		color: #ff3e00;
		text-transform: uppercase;
		font-size: 4em;
		font-weight: 100;
	}
</style>