<script>
import ConnectScreen from './ConnectScreen.svelte';
import Secret from './Secret.svelte';
import Join from './Join.svelte';
import Board from './Board.svelte';
import Messages from './Messages.svelte';
import { Client } from './client.js';
import { setContext } from 'svelte';
import { subscribeToClient, connected } from './stores';
import { messageStore, subscribe } from './messageStore.js';

const client = new Client('localhost', 4545);
subscribeToClient(client);
subscribe(client);
setContext('client', client);
let showMessages;
$: showMessages = ($messageStore).length > 0;

</script>

<main>
{#if !$connected}
	<ConnectScreen />
{:else if showMessages}
	<Messages />
{:else}
	<Board />
{/if}

</main>
<style>
	main {
		text-align: center;
		width: 100%;
		height: 100%;
	}
</style>