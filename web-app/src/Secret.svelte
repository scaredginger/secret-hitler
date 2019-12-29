<script>
let displayed = false;
export let ondismiss;

function show() {
    displayed = true;
}

function hide() {
    displayed = false;
}

function onclose() {
    if (ondismiss) {
        ondismiss();
    }
}
</script>

<main>
<div id="close-button">
<button on:click={onclose}>
X
</button>
</div>

<slot name="title">
</slot>
<slot name="info">
</slot>
{#if displayed}
<slot name="hidden">
<p>No information to display</p>
</slot>
{/if}

<div id="revealArea" on:mousedown={show} on:mouseup={hide} on:touchstart={show} on:touchend={hide}><p>Reveal</p></div>
</main>

<style>
#close-button {
    position: relative;
    top: 0;
    right: 0;
    left: 100%;
    transform: translateX(-100%);
    height: 10%;
    width: 100%;
    text-align: right;
    font-size: 2em;
}

slot {
    position: relative;
    height: 70%;
}
main {
    height: 100%;
}

#revealArea {
    position: absolute;
    bottom: 0;
    height: 20%;
    width: 100%;
    background: #333;
    user-select: none;
}

#revealArea:active {
    background: #222;
    border: 1px solid #ccc;
    height: calc(20% - 2px);
    width: calc(100% - 2px);
}

#revealArea p {
    vertical-align: middle;
    font-size: 3em;
}

</style>