import React, { useState } from 'react';
import { LineChart, Line, BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { Download, Info } from 'lucide-react';

const LMBenchAnalyzer = () => {
  const [activeTab, setActiveTab] = useState('overview');

  // System Information
  const sysInfo = {
    hostname: "dell-Precision-3660",
    os: "Ubuntu 24.04.1",
    kernel: "6.14.0-33-generic",
    cpu: "5780 MHz (0.1730 ns clock)",
    processors: 32,
    memory: "8192 MB"
  };

  // Syscall and Context Switch Latencies (microseconds)
  const syscallData = [
    { name: 'Simple syscall', value: 0.0556 },
    { name: 'Simple read', value: 0.0881 },
    { name: 'Simple write', value: 0.0749 },
    { name: 'Simple stat', value: 0.2713 },
    { name: 'Simple fstat', value: 0.1064 },
    { name: 'Open/close', value: 0.5643 },
    { name: 'Signal handler', value: 0.5884 },
    { name: 'Protection fault', value: 0.2872 }
  ];

  // Process operations (microseconds)
  const processData = [
    { name: 'Fork+exit', value: 291.40 },
    { name: 'Fork+execve', value: 1307.00 },
    { name: 'Fork+/bin/sh', value: 2651.50 }
  ];

  // IPC Latencies (microseconds)
  const ipcData = [
    { name: 'Pipe', value: 2.87 },
    { name: 'AF_UNIX stream', value: 2.81 },
    { name: 'UDP localhost', value: 4.31 },
    { name: 'TCP localhost', value: 5.56 },
    { name: 'RPC/UDP', value: 5.55 },
    { name: 'RPC/TCP', value: 6.88 },
    { name: 'TCP connect', value: 7.70 }
  ];

  // Integer Operations (nanoseconds)
  const intOpsData = [
    { name: 'Bit', value: 0.13 },
    { name: 'Add', value: 0.00 },
    { name: 'Mul', value: 0.54 },
    { name: 'Div', value: 1.92 },
    { name: 'Mod', value: 2.96 }
  ];

  // Float Operations (nanoseconds)
  const floatOpsData = [
    { name: 'Add', value: 0.35 },
    { name: 'Mul', value: 0.69 },
    { name: 'Div', value: 1.91 }
  ];

  // Memory Bandwidth (MB/sec) - selected sizes
  const memBandwidthData = [
    { size: '512B', read: 1751.79, write: 44741.90, mmap: 103710.61 },
    { size: '1KB', read: 3304.72, write: 45847.59, mmap: 82080.72 },
    { size: '4KB', read: 11269.36, write: 43778.40, mmap: 83253.77 },
    { size: '64KB', read: 29412.61, write: 40557.40, mmap: 67030.27 },
    { size: '1MB', read: 29947.63, write: 38881.81, mmap: 66126.41 },
    { size: '16MB', read: 22052.07, write: 32151.46, mmap: 48647.24 },
    { size: '128MB', read: 7763.64, write: 10359.50, mmap: 12067.77 },
    { size: '1GB', read: 7785.36, write: 9909.02, mmap: 14563.95 }
  ];

  // Memory Copy Bandwidth (MB/sec) - selected sizes
  const memCopyData = [
    { size: '512B', bcopy: 188094.24, aligned: 205774.51, bzero: 294851.06 },
    { size: '4KB', bcopy: 264482.44, aligned: 266082.84, bzero: 322042.74 },
    { size: '64KB', bcopy: 79480.47, aligned: 76285.61, bzero: 88474.79 },
    { size: '1MB', bcopy: 59874.90, aligned: 57574.87, bzero: 89822.67 },
    { size: '16MB', bcopy: 22725.14, aligned: 22742.16, bzero: 53773.13 },
    { size: '128MB', bcopy: 13470.27, aligned: 13697.08, bzero: 12121.17 }
  ];

  // Memory Latency - L1/L2/L3/RAM (nanoseconds, stride 128)
  const memLatencyData = [
    { size: '4KB', latency: 0.867 },
    { size: '8KB', latency: 0.867 },
    { size: '32KB', latency: 0.868 },
    { size: '64KB', latency: 2.777 },
    { size: '128KB', latency: 2.781 },
    { size: '256KB', latency: 2.776 },
    { size: '512KB', latency: 2.821 },
    { size: '1MB', latency: 1.056 },
    { size: '2MB', latency: 1.260 },
    { size: '4MB', latency: 1.619 },
    { size: '8MB', latency: 3.401 },
    { size: '16MB', latency: 1.715 },
    { size: '32MB', latency: 8.817 },
    { size: '64MB', latency: 7.996 },
    { size: '128MB', latency: 17.298 },
    { size: '512MB', latency: 10.130 },
    { size: '8GB', latency: 9.944 }
  ];

  // Network Bandwidth
  const networkBandwidth = [
    { size: '1KB', bandwidth: 6.06 },
    { size: '64KB', bandwidth: 341.33 },
    { size: '128KB', bandwidth: 654.80 },
    { size: '1MB', bandwidth: 4543.59 },
    { size: '10MB', bandwidth: 10737.61 }
  ];

  const renderOverview = () => (
    <div className="space-y-6">
      <div className="bg-blue-50 border border-blue-200 rounded-lg p-4">
        <div className="flex items-start gap-2">
          <Info className="w-5 h-5 text-blue-600 mt-0.5" />
          <div>
            <h3 className="font-semibold text-blue-900">System Information</h3>
            <div className="grid grid-cols-2 gap-x-6 gap-y-2 mt-2 text-sm">
              <div><span className="font-medium">Hostname:</span> {sysInfo.hostname}</div>
              <div><span className="font-medium">OS:</span> {sysInfo.os}</div>
              <div><span className="font-medium">Kernel:</span> {sysInfo.kernel}</div>
              <div><span className="font-medium">CPU:</span> {sysInfo.cpu}</div>
              <div><span className="font-medium">Processors:</span> {sysInfo.processors}</div>
              <div><span className="font-medium">Memory:</span> {sysInfo.memory}</div>
            </div>
          </div>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <div className="bg-white border rounded-lg p-4">
          <h4 className="font-semibold mb-2">Fastest Syscall</h4>
          <p className="text-2xl font-bold text-green-600">0.056 µs</p>
          <p className="text-sm text-gray-600">Simple syscall</p>
        </div>
        <div className="bg-white border rounded-lg p-4">
          <h4 className="font-semibold mb-2">Memory Bandwidth</h4>
          <p className="text-2xl font-bold text-blue-600">29.9 GB/s</p>
          <p className="text-sm text-gray-600">Peak read @ 1MB</p>
        </div>
        <div className="bg-white border rounded-lg p-4">
          <h4 className="font-semibold mb-2">L1 Cache Latency</h4>
          <p className="text-2xl font-bold text-purple-600">0.87 ns</p>
          <p className="text-sm text-gray-600">Stride 128</p>
        </div>
      </div>
    </div>
  );

  const renderSyscalls = () => (
    <div className="space-y-6">
      <div>
        <h3 className="text-lg font-semibold mb-4">System Call Latencies</h3>
        <ResponsiveContainer width="100%" height={300}>
          <BarChart data={syscallData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="name" angle={-45} textAnchor="end" height={100} />
            <YAxis label={{ value: 'Microseconds (µs)', angle: -90, position: 'insideLeft' }} />
            <Tooltip />
            <Bar dataKey="value" fill="#3b82f6" />
          </BarChart>
        </ResponsiveContainer>
      </div>

      <div>
        <h3 className="text-lg font-semibold mb-4">Process Operations</h3>
        <ResponsiveContainer width="100%" height={250}>
          <BarChart data={processData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="name" />
            <YAxis label={{ value: 'Microseconds (µs)', angle: -90, position: 'insideLeft' }} />
            <Tooltip />
            <Bar dataKey="value" fill="#8b5cf6" />
          </BarChart>
        </ResponsiveContainer>
      </div>

      <div>
        <h3 className="text-lg font-semibold mb-4">IPC Latencies</h3>
        <ResponsiveContainer width="100%" height={300}>
          <BarChart data={ipcData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="name" angle={-45} textAnchor="end" height={80} />
            <YAxis label={{ value: 'Microseconds (µs)', angle: -90, position: 'insideLeft' }} />
            <Tooltip />
            <Bar dataKey="value" fill="#10b981" />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );

  const renderCPU = () => (
    <div className="space-y-6">
      <div>
        <h3 className="text-lg font-semibold mb-4">Integer Operations (Lower is Better)</h3>
        <ResponsiveContainer width="100%" height={250}>
          <BarChart data={intOpsData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="name" />
            <YAxis label={{ value: 'Nanoseconds (ns)', angle: -90, position: 'insideLeft' }} />
            <Tooltip />
            <Bar dataKey="value" fill="#f59e0b" />
          </BarChart>
        </ResponsiveContainer>
      </div>

      <div>
        <h3 className="text-lg font-semibold mb-4">Float Operations (Lower is Better)</h3>
        <ResponsiveContainer width="100%" height={250}>
          <BarChart data={floatOpsData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="name" />
            <YAxis label={{ value: 'Nanoseconds (ns)', angle: -90, position: 'insideLeft' }} />
            <Tooltip />
            <Bar dataKey="value" fill="#ef4444" />
          </BarChart>
        </ResponsiveContainer>
      </div>
    </div>
  );

  const renderMemory = () => (
    <div className="space-y-6">
      <div>
        <h3 className="text-lg font-semibold mb-4">Memory Read/Write Bandwidth</h3>
        <ResponsiveContainer width="100%" height={350}>
          <LineChart data={memBandwidthData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="size" />
            <YAxis label={{ value: 'MB/sec', angle: -90, position: 'insideLeft' }} />
            <Tooltip />
            <Legend />
            <Line type="monotone" dataKey="read" stroke="#3b82f6" strokeWidth={2} />
            <Line type="monotone" dataKey="write" stroke="#10b981" strokeWidth={2} />
            <Line type="monotone" dataKey="mmap" stroke="#8b5cf6" strokeWidth={2} />
          </LineChart>
        </ResponsiveContainer>
      </div>

      <div>
        <h3 className="text-lg font-semibold mb-4">Memory Copy Operations</h3>
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={memCopyData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="size" />
            <YAxis label={{ value: 'MB/sec', angle: -90, position: 'insideLeft' }} />
            <Tooltip />
            <Legend />
            <Line type="monotone" dataKey="bcopy" stroke="#f59e0b" strokeWidth={2} />
            <Line type="monotone" dataKey="aligned" stroke="#ef4444" strokeWidth={2} />
            <Line type="monotone" dataKey="bzero" stroke="#06b6d4" strokeWidth={2} />
          </LineChart>
        </ResponsiveContainer>
      </div>

      <div>
        <h3 className="text-lg font-semibold mb-4">Memory Latency (Cache Hierarchy)</h3>
        <ResponsiveContainer width="100%" height={350}>
          <LineChart data={memLatencyData}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="size" />
            <YAxis label={{ value: 'Nanoseconds (ns)', angle: -90, position: 'insideLeft' }} />
            <Tooltip />
            <Line type="monotone" dataKey="latency" stroke="#8b5cf6" strokeWidth={2} />
          </LineChart>
        </ResponsiveContainer>
        <div className="mt-4 p-4 bg-gray-50 rounded">
          <p className="text-sm text-gray-700">
            <strong>Cache observations:</strong> L1 cache (~0.87ns), L2/L3 transition (~2.8ns), 
            Main memory access varies (1-17ns depending on size), showing NUMA effects on this 32-core system.
          </p>
        </div>
      </div>
    </div>
  );

  const renderNetwork = () => (
    <div className="space-y-6">
      <div>
        <h3 className="text-lg font-semibold mb-4">Socket Bandwidth (Localhost)</h3>
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={networkBandwidth}>
            <CartesianGrid strokeDasharray="3 3" />
            <XAxis dataKey="size" />
            <YAxis label={{ value: 'MB/sec', angle: -90, position: 'insideLeft' }} />
            <Tooltip />
            <Line type="monotone" dataKey="bandwidth" stroke="#10b981" strokeWidth={2} />
          </LineChart>
        </ResponsiveContainer>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        <div className="bg-white border rounded-lg p-4">
          <h4 className="font-semibold mb-2">AF_UNIX Stream</h4>
          <p className="text-2xl font-bold text-blue-600">18.3 GB/s</p>
          <p className="text-sm text-gray-600">Bandwidth</p>
        </div>
        <div className="bg-white border rounded-lg p-4">
          <h4 className="font-semibold mb-2">Pipe</h4>
          <p className="text-2xl font-bold text-green-600">4.8 GB/s</p>
          <p className="text-sm text-gray-600">Bandwidth</p>
        </div>
      </div>
    </div>
  );

  const tabs = [
    { id: 'overview', label: 'Overview' },
    { id: 'syscalls', label: 'System Calls & IPC' },
    { id: 'cpu', label: 'CPU Operations' },
    { id: 'memory', label: 'Memory' },
    { id: 'network', label: 'Network' }
  ];

  return (
    <div className="w-full h-screen bg-gray-50 flex flex-col">
      <div className="bg-white border-b shadow-sm">
        <div className="max-w-7xl mx-auto px-4 py-4">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-2xl font-bold text-gray-900">LMBench Results Analyzer</h1>
              <p className="text-sm text-gray-600 mt-1">{sysInfo.hostname} - {sysInfo.os}</p>
            </div>
            <button className="flex items-center gap-2 px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700">
              <Download className="w-4 h-4" />
              Export Report
            </button>
          </div>
        </div>
      </div>

      <div className="bg-white border-b">
        <div className="max-w-7xl mx-auto px-4">
          <div className="flex gap-1">
            {tabs.map(tab => (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id)}
                className={`px-4 py-3 font-medium transition-colors ${
                  activeTab === tab.id
                    ? 'text-blue-600 border-b-2 border-blue-600'
                    : 'text-gray-600 hover:text-gray-900'
                }`}
              >
                {tab.label}
              </button>
            ))}
          </div>
        </div>
      </div>

      <div className="flex-1 overflow-auto">
        <div className="max-w-7xl mx-auto px-4 py-6">
          {activeTab === 'overview' && renderOverview()}
          {activeTab === 'syscalls' && renderSyscalls()}
          {activeTab === 'cpu' && renderCPU()}
          {activeTab === 'memory' && renderMemory()}
          {activeTab === 'network' && renderNetwork()}
        </div>
      </div>
    </div>
  );
};

export default LMBenchAnalyzer;