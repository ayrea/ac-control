import { ACControlPanel } from '@/components/ACControlPanel'

function App() {
  return (
    <div className="dark bg-background min-h-svh w-full">
      <div className="mx-auto flex min-h-svh max-w-md flex-col px-4 py-6">
        <ACControlPanel />
      </div>
    </div>
  )
}

export default App
