// ...existing code...
import { Button } from '@/components/ui/button';
import { Play, Pause, RotateCcw } from 'lucide-react';
// ...existing code...
// Supprimer: import { SimulationControls } from '@/components/SimulationControls';

function App() {
  // ...existing code...

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 to-slate-800">
      {/* ...existing code... */}
      
      <main className="container mx-auto px-4 py-8">
        <div className="mb-6 flex gap-4">
          <Button
            variant={isRunning ? "destructive" : "default"}
            onClick={isRunning ? pause : start}
          >
            {isRunning ? <Pause className="w-4 h-4 mr-2" /> : <Play className="w-4 h-4 mr-2" />}
            {isRunning ? 'Pause' : 'Start'}
          </Button>
          <Button variant="outline" onClick={reset}>
            <RotateCcw className="w-4 h-4 mr-2" />
            Reset
          </Button>
        </div>

        {/* Supprimer: <SimulationControls /> */}
        
        {/* ...existing code... */}
      </main>
    </div>
  );
}

