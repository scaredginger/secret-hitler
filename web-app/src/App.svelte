<script>
import ConnectScreen from './ConnectScreen.svelte';
import Reveal from './Reveal.svelte';
import Join from './Join.svelte';
import { Connection } from './connection.js';
let connection = new Connection();
let data = 'hello world'

function hideData() {
	console.log(data)
	data = null;
}

</script>

<main>
<!--
{#if connection.disconnected}
	<ConnectScreen connection={connection} />
{/if}
-->

{#if data}
	<Reveal back={hideData}>
	<p>
		{data}
	</p>
	</Reveal>
{/if}

</main>
<style>
	main {
		text-align: center;
		width: 100%;
		height: 100%;
	}
</style>