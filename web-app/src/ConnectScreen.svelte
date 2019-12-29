<script>
import Join from './Join.svelte';
import Create from './Create.svelte';
import { getContext } from 'svelte';

const client = getContext('client');
let mode = '';
function createGame() {
	mode = 'create';
}

function joinGame() {
	mode = 'join';
}

</script>

<main>
	{#if mode === 'join'}
		<Join />
	{:else if mode === 'create'}
		<Create />
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