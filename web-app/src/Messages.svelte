<script>
import { getContext } from 'svelte';
import Secret from './Secret.svelte';
import TextMessage from './TextMessage.svelte';
import SecretDecision from './SecretDecision.svelte';
import Decision from './Decision.svelte';
import DecisionAlert from './DecisionAlert.svelte';
import { messageStore, popMessage } from './messageStore';

const client = getContext('client');

let currentMessage;
$: currentMessage = $messageStore[0];

function handleDecision(decision) {
  currentMessage.handler(decision);
  popMessage();
}

</script>

<main>
{#if currentMessage.type === 'text'}
  <TextMessage ondismiss={popMessage}>
    <h3 slot="title">
      { currentMessage.title }
    </h3>
    <div slot="text">
      <p>
        { currentMessage.text }
      </p>
    </div>
  </TextMessage>
{:else if currentMessage.type === 'secret'}
  <Secret ondismiss={popMessage}>
    <h3 slot="title">
      { currentMessage.title }
    </h3>
    <p slot="info">
      {currentMessage.info}
    </p>
    <div slot="hidden">
      <p>
        { currentMessage.text }
      </p>
    </div>
  </Secret>
{:else if currentMessage.type === 'secretDecision'}
  <SecretDecision options={currentMessage.options} ondecision={handleDecision}>
    <h3 slot="title">
      {currentMessage.title}
    </h3>
    <p slot="info">
      {currentMessage.info}
    </p>
  </SecretDecision>
{:else if currentMessage.type === 'decision'}
  <Decision options={currentMessage.options} ondecision={handleDecision}>
    <h3 slot="title">
      {currentMessage.title}
    </h3>
    <p slot="info">
      {currentMessage.info}
    </p>
  </Decision>
{:else if currentMessage.type === 'decisionAlert'}
  <DecisionAlert options={currentMessage.options} ondismiss={popMessage}>
    <h3 slot="title">
      {currentMessage.title}
    </h3>
    <p slot="info">
      {currentMessage.info}
    </p>
  </DecisionAlert>
{:else if currentMessage.type === 'electionResult'}
  <TextMessage ondismiss={popMessage}>
    <h3 slot="title">
      { currentMessage.title }
    </h3>
    <div slot="text">
      <h4>Ja</h4>
      <ul>
        {#each currentMessage.votes.filter(item => item.vote === 'ja') as item}
          <li>
            { item.player }
          </li>
        {/each}
      </ul>
      <h4>Nein</h4>
      <ul>
        {#each currentMessage.votes.filter(item => item.vote === 'nein') as item}
          <li>
            { item.player }
          </li>
        {/each}
      </ul>
    </div>
  </TextMessage>
{:else if currentMessage.type === 'team'}
  <Secret ondismiss={popMessage}>
    <h3 slot="title">
      Team
    </h3>
    <div slot="hidden">
      <p>Your role is {currentMessage.role}</p>
      {#if currentMessage.roles}
        <ul>
          {#each currentMessage.roles.filter(item => item.role === 'liberal') as item}
            <li>{item.player}: {item.role}</li>
          {/each}
          {#each currentMessage.roles.filter(item => item.role === 'fascist') as item}
            <li>{item.player}: {item.role}</li>
          {/each}
          {#each currentMessage.roles.filter(item => item.role === 'hitler') as item}
            <li>{item.player}: {item.role}</li>
          {/each}
        </ul>
      {/if}
    </div>
  </Secret>
{/if}
</main>
