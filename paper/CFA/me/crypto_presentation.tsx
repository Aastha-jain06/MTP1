import React, { useState } from 'react';
import { ChevronLeft, ChevronRight, Shield, Lock, Key, FileCheck, Network, AlertTriangle, Mail, BookOpen } from 'lucide-react';

const CryptographyPresentation = () => {
  const [currentSlide, setCurrentSlide] = useState(0);

  const slides = [
    {
      title: "Introduction to Cryptography",
      subtitle: "Making Secrets Safe in the Digital World",
      icon: Shield,
      content: (
        <div className="text-center space-y-8">
          <Shield className="w-40 h-40 mx-auto text-blue-600" />
          <div className="space-y-4">
            <p className="text-3xl font-bold text-gray-800">Welcome!</p>
            <p className="text-xl text-gray-600">Let's learn how to keep information secure</p>
          </div>
        </div>
      )
    },
    {
      title: "Why Do We Need Cryptography?",
      icon: BookOpen,
      content: (
        <div className="space-y-6">
          <p className="text-xl text-gray-700">Think about your daily life online...</p>
          
          <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
            <div className="bg-blue-50 p-6 rounded-lg border-2 border-blue-200">
              <h3 className="text-2xl font-bold mb-4 text-blue-900">🏦 Online Banking</h3>
              <p className="text-gray-700">You send money to a friend. How does the bank know it's really you? How do they keep your account number secret?</p>
            </div>
            
            <div className="bg-green-50 p-6 rounded-lg border-2 border-green-200">
              <h3 className="text-2xl font-bold mb-4 text-green-900">📧 Email</h3>
              <p className="text-gray-700">You email your doctor about private health information. How do you know no one else can read it?</p>
            </div>
            
            <div className="bg-purple-50 p-6 rounded-lg border-2 border-purple-200">
              <h3 className="text-2xl font-bold mb-4 text-purple-900">🛒 Online Shopping</h3>
              <p className="text-gray-700">You enter your credit card on a website. How does it stay safe from hackers?</p>
            </div>
            
            <div className="bg-orange-50 p-6 rounded-lg border-2 border-orange-200">
              <h3 className="text-2xl font-bold mb-4 text-orange-900">💬 Messaging</h3>
              <p className="text-gray-700">You chat with friends on WhatsApp. How do they keep your conversations private?</p>
            </div>
          </div>
          
          <div className="bg-yellow-100 p-6 rounded-lg border-2 border-yellow-400">
            <p className="text-xl font-bold text-center text-gray-800">Cryptography is the answer to all these questions! ✨</p>
          </div>
        </div>
      )
    },
    {
      title: "What is Cryptography?",
      icon: Lock,
      content: (
        <div className="space-y-6">
          <div className="bg-blue-50 p-8 rounded-lg border-2 border-blue-300">
            <p className="text-2xl text-center text-gray-800 leading-relaxed">
              <strong className="text-blue-900">Cryptography</strong> is the art of writing or solving <strong className="text-blue-900">codes</strong> to keep information <strong className="text-blue-900">secret and safe</strong>.
            </p>
          </div>

          <div className="bg-white p-6 rounded-lg border-2 border-gray-200">
            <h3 className="text-2xl font-bold mb-4 text-gray-800">Simple Example:</h3>
            <div className="space-y-4">
              <div className="bg-green-50 p-4 rounded">
                <p className="text-lg"><strong>Original Message:</strong> "HELLO"</p>
              </div>
              <div className="text-center text-3xl text-gray-400">↓ Transform ↓</div>
              <div className="bg-red-50 p-4 rounded">
                <p className="text-lg"><strong>Secret Code:</strong> "KHOOR" (shift each letter by 3)</p>
              </div>
              <div className="text-center text-3xl text-gray-400">↓ Reverse ↓</div>
              <div className="bg-green-50 p-4 rounded">
                <p className="text-lg"><strong>Back to Original:</strong> "HELLO"</p>
              </div>
            </div>
          </div>

          <div className="bg-purple-50 p-4 rounded-lg">
            <p className="text-lg text-gray-700"><strong>Key Point:</strong> Only people who know the "rule" (shift by 3) can decode the message!</p>
          </div>
        </div>
      )
    },
    {
      title: "The Four Goals of Cryptography",
      icon: Shield,
      content: (
        <div className="space-y-6">
          <p className="text-xl text-gray-700 text-center mb-6">Cryptography helps us achieve four important things:</p>
          
          <div className="space-y-4">
            <div className="bg-blue-50 p-6 rounded-lg border-l-4 border-blue-600">
              <h3 className="text-2xl font-bold mb-3 text-blue-900">🔒 1. Confidentiality (Privacy)</h3>
              <p className="text-lg text-gray-700 mb-2"><strong>What it means:</strong> Only the right people can read the message</p>
              <p className="text-gray-600"><strong>Example:</strong> Your password is hidden when you type it - dots (••••) instead of letters</p>
            </div>

            <div className="bg-green-50 p-6 rounded-lg border-l-4 border-green-600">
              <h3 className="text-2xl font-bold mb-3 text-green-900">✅ 2. Integrity (No Tampering)</h3>
              <p className="text-lg text-gray-700 mb-2"><strong>What it means:</strong> The message hasn't been changed</p>
              <p className="text-gray-600"><strong>Example:</strong> If someone tries to change "$10" to "$1000" in your bank transfer, the system detects it</p>
            </div>

            <div className="bg-purple-50 p-6 rounded-lg border-l-4 border-purple-600">
              <h3 className="text-2xl font-bold mb-3 text-purple-900">👤 3. Authentication (Identity)</h3>
              <p className="text-lg text-gray-700 mb-2"><strong>What it means:</strong> You can verify who sent the message</p>
              <p className="text-gray-600"><strong>Example:</strong> Your fingerprint or face unlock proves it's really you accessing your phone</p>
            </div>

            <div className="bg-orange-50 p-6 rounded-lg border-l-4 border-orange-600">
              <h3 className="text-2xl font-bold mb-3 text-orange-900">📝 4. Non-repudiation (Can't Deny)</h3>
              <p className="text-lg text-gray-700 mb-2"><strong>What it means:</strong> The sender can't deny they sent the message</p>
              <p className="text-gray-600"><strong>Example:</strong> Your digital signature on a contract proves you agreed to it</p>
            </div>
          </div>
        </div>
      )
    },
    {
      title: "The Three Main Types",
      icon: Key,
      content: (
        <div className="space-y-6">
          <p className="text-xl text-gray-700 text-center">There are three main ways to do cryptography:</p>
          
          <div className="space-y-4">
            <div className="bg-gradient-to-r from-purple-100 to-purple-50 p-6 rounded-lg shadow-lg border-2 border-purple-300">
              <div className="flex items-center gap-4 mb-3">
                <div className="bg-purple-600 text-white w-12 h-12 rounded-full flex items-center justify-center text-2xl font-bold">1</div>
                <h3 className="text-2xl font-bold text-purple-900">Symmetric (Same Key)</h3>
              </div>
              <p className="text-lg text-gray-700 mb-2">One key does everything - both locking and unlocking</p>
              <p className="text-gray-600 italic">Like a house key: same key locks and unlocks your door 🔑</p>
            </div>
            
            <div className="bg-gradient-to-r from-green-100 to-green-50 p-6 rounded-lg shadow-lg border-2 border-green-300">
              <div className="flex items-center gap-4 mb-3">
                <div className="bg-green-600 text-white w-12 h-12 rounded-full flex items-center justify-center text-2xl font-bold">2</div>
                <h3 className="text-2xl font-bold text-green-900">Asymmetric (Two Keys)</h3>
              </div>
              <p className="text-lg text-gray-700 mb-2">Two different keys - one locks, the other unlocks</p>
              <p className="text-gray-600 italic">Like a mailbox: anyone can drop mail in (public), but only you have the key to open it (private) 📬</p>
            </div>
            
            <div className="bg-gradient-to-r from-orange-100 to-orange-50 p-6 rounded-lg shadow-lg border-2 border-orange-300">
              <div className="flex items-center gap-4 mb-3">
                <div className="bg-orange-600 text-white w-12 h-12 rounded-full flex items-center justify-center text-2xl font-bold">3</div>
                <h3 className="text-2xl font-bold text-orange-900">Hash Functions (One-Way)</h3>
              </div>
              <p className="text-lg text-gray-700 mb-2">Takes any message and creates a unique "fingerprint" - can't be reversed</p>
              <p className="text-gray-600 italic">Like a blender: easy to blend fruits into smoothie, impossible to get the fruits back 🥤</p>
            </div>
          </div>
        </div>
      )
    },
    {
      title: "Symmetric Cryptography Explained",
      icon: Lock,
      content: (
        <div className="space-y-6">
          <div className="bg-purple-50 p-6 rounded-lg border-2 border-purple-300">
            <h3 className="text-2xl font-bold mb-4 text-purple-900">The "Same Key" Method</h3>
            <p className="text-lg text-gray-700">Both sender and receiver use the SAME secret key</p>
          </div>

          <div className="bg-white p-6 rounded-lg border-2 border-gray-300">
            <h3 className="text-xl font-bold mb-4 text-gray-800">Step-by-Step Example:</h3>
            <div className="space-y-4">
              <div className="flex items-center gap-4">
                <div className="bg-blue-600 text-white w-10 h-10 rounded-full flex items-center justify-center font-bold">1</div>
                <div className="flex-1 bg-blue-50 p-3 rounded">
                  <p><strong>Alice</strong> wants to send a secret message: "MEET AT NOON"</p>
                </div>
              </div>
              
              <div className="flex items-center gap-4">
                <div className="bg-blue-600 text-white w-10 h-10 rounded-full flex items-center justify-center font-bold">2</div>
                <div className="flex-1 bg-blue-50 p-3 rounded">
                  <p>Alice and Bob <strong>already share</strong> a secret key: "SHIFT-5"</p>
                </div>
              </div>
              
              <div className="flex items-center gap-4">
                <div className="bg-blue-600 text-white w-10 h-10 rounded-full flex items-center justify-center font-bold">3</div>
                <div className="flex-1 bg-red-50 p-3 rounded">
                  <p>Alice uses the key to encrypt: "RJJY FY STTS"</p>
                </div>
              </div>
              
              <div className="flex items-center gap-4">
                <div className="bg-blue-600 text-white w-10 h-10 rounded-full flex items-center justify-center font-bold">4</div>
                <div className="flex-1 bg-yellow-50 p-3 rounded">
                  <p>Alice sends the encrypted message (safe to send over internet!)</p>
                </div>
              </div>
              
              <div className="flex items-center gap-4">
                <div className="bg-blue-600 text-white w-10 h-10 rounded-full flex items-center justify-center font-bold">5</div>
                <div className="flex-1 bg-green-50 p-3 rounded">
                  <p><strong>Bob</strong> uses the SAME key "SHIFT-5" to decrypt: "MEET AT NOON"</p>
                </div>
              </div>
            </div>
          </div>

          <div className="grid grid-cols-2 gap-4">
            <div className="bg-green-50 p-4 rounded-lg">
              <h4 className="font-bold text-green-900 mb-2">✅ Good Points:</h4>
              <p className="text-sm text-gray-700">Very fast! Works great for large files</p>
            </div>
            
            <div className="bg-red-50 p-4 rounded-lg">
              <h4 className="font-bold text-red-900 mb-2">❌ Challenge:</h4>
              <p className="text-sm text-gray-700">How do Alice and Bob share the key safely in the first place?</p>
            </div>
          </div>

          <div className="bg-gray-100 p-4 rounded">
            <p className="font-semibold text-gray-800">Real Example: <span className="text-blue-600">AES</span> - Used to encrypt your phone's data!</p>
          </div>
        </div>
      )
    },
    {
      title: "Asymmetric Cryptography Explained",
      icon: Key,
      content: (
        <div className="space-y-6">
          <div className="bg-green-50 p-6 rounded-lg border-2 border-green-300">
            <h3 className="text-2xl font-bold mb-4 text-green-900">The "Two Keys" Method</h3>
            <p className="text-lg text-gray-700">Everyone has TWO keys: a Public Key (share with everyone) and a Private Key (keep secret!)</p>
          </div>

          <div className="bg-white p-6 rounded-lg border-2 border-gray-300">
            <h3 className="text-xl font-bold mb-4 text-gray-800">The Mailbox Analogy:</h3>
            <div className="space-y-4">
              <div className="bg-blue-100 p-4 rounded-lg">
                <h4 className="font-bold text-blue-900 mb-2">🌍 Public Key (Mailbox Slot)</h4>
                <p className="text-gray-700">Like the slot in a mailbox - anyone can drop a letter in, but they can't read what's already inside</p>
              </div>
              
              <div className="bg-red-100 p-4 rounded-lg">
                <h4 className="font-bold text-red-900 mb-2">🔐 Private Key (Mailbox Key)</h4>
                <p className="text-gray-700">Only YOU have this key. It's the only way to open the mailbox and read the letters inside</p>
              </div>
            </div>
          </div>

          <div className="bg-purple-50 p-6 rounded-lg border-2 border-purple-200">
            <h3 className="text-xl font-bold mb-4 text-purple-900">How Bob Sends Alice a Secret:</h3>
            <div className="space-y-3">
              <div className="flex items-start gap-3">
                <span className="bg-purple-600 text-white px-3 py-1 rounded font-bold">1</span>
                <p className="flex-1 text-gray-700">Bob gets Alice's <strong>public key</strong> (she shares it openly - no problem!)</p>
              </div>
              <div className="flex items-start gap-3">
                <span className="bg-purple-600 text-white px-3 py-1 rounded font-bold">2</span>
                <p className="flex-1 text-gray-700">Bob uses Alice's public key to <strong>encrypt</strong> his message</p>
              </div>
              <div className="flex items-start gap-3">
                <span className="bg-purple-600 text-white px-3 py-1 rounded font-bold">3</span>
                <p className="flex-1 text-gray-700">Bob sends the encrypted message (safe over internet!)</p>
              </div>
              <div className="flex items-start gap-3">
                <span className="bg-purple-600 text-white px-3 py-1 rounded font-bold">4</span>
                <p className="flex-1 text-gray-700">Only Alice can <strong>decrypt</strong> it using her <strong>private key</strong></p>
              </div>
            </div>
          </div>

          <div className="bg-yellow-100 p-4 rounded-lg border-2 border-yellow-400">
            <p className="font-bold text-gray-800">🎉 The Magic: No need to secretly share a key beforehand!</p>
          </div>

          <div className="bg-gray-100 p-4 rounded">
            <p className="font-semibold text-gray-800">Real Example: <span className="text-green-600">RSA</span> - Used when you see the padlock 🔒 in your browser!</p>
          </div>
        </div>
      )
    },
    {
      title: "Hash Functions Explained",
      icon: FileCheck,
      content: (
        <div className="space-y-6">
          <div className="bg-orange-50 p-6 rounded-lg border-2 border-orange-300">
            <h3 className="text-2xl font-bold mb-3 text-orange-900">The "Digital Fingerprint"</h3>
            <p className="text-lg text-gray-700">Takes ANY message and creates a unique, fixed-size "fingerprint" of it</p>
          </div>

          <div className="bg-white p-6 rounded-lg border-2 border-gray-300">
            <h3 className="text-xl font-bold mb-4 text-gray-800">See It In Action:</h3>
            <div className="space-y-4 font-mono text-sm">
              <div className="bg-blue-50 p-4 rounded">
                <p className="text-gray-600 mb-1">Input Message:</p>
                <p className="text-lg font-bold">"Hello"</p>
              </div>
              <div className="text-center text-2xl">↓ Hash Function ↓</div>
              <div className="bg-green-50 p-4 rounded">
                <p className="text-gray-600 mb-1">Hash (fingerprint):</p>
                <p className="text-xs break-all font-bold">185f8db32271fe25f561a6fc938b2e264306ec304eda518007d1764826381969</p>
              </div>
            </div>
            
            <div className="mt-4 bg-yellow-50 p-4 rounded">
              <p className="text-gray-700">Even a tiny change creates a completely different hash:</p>
              <div className="mt-2 space-y-2 text-xs">
                <p><strong>"Hello"</strong> → 185f8db32271...</p>
                <p><strong>"hello"</strong> → 2cf24dba5fb0... (totally different!)</p>
              </div>
            </div>
          </div>

          <div className="bg-purple-50 p-6 rounded-lg">
            <h3 className="text-xl font-bold mb-3 text-purple-900">Key Properties (Important!):</h3>
            <div className="space-y-2 text-gray-700">
              <p>✅ <strong>Same input = Same output:</strong> "Hello" always gives the same hash</p>
              <p>✅ <strong>One-way only:</strong> Can't reverse the hash to get "Hello" back</p>
              <p>✅ <strong>Unique-ish:</strong> Nearly impossible for two different messages to have the same hash</p>
              <p>✅ <strong>Fixed size:</strong> Whether you hash "Hi" or an entire book, the hash is the same length</p>
            </div>
          </div>

          <div className="bg-green-50 p-5 rounded-lg">
            <h4 className="font-bold text-green-900 mb-2">Real-World Uses:</h4>
            <div className="space-y-2 text-gray-700">
              <p>🔐 <strong>Password storage:</strong> Websites store the hash of your password, not the actual password!</p>
              <p>📦 <strong>File verification:</strong> Download a file, check its hash to make sure it wasn't corrupted or tampered with</p>
              <p>⛓️ <strong>Blockchain:</strong> Each block contains a hash of the previous block, creating an unbreakable chain</p>
            </div>
          </div>

          <div className="bg-gray-100 p-4 rounded">
            <p className="font-semibold text-gray-800">Popular Algorithm: <span className="text-orange-600">SHA-256</span> - Used in Bitcoin!</p>
          </div>
        </div>
      )
    },
    {
      title: "Digital Signatures",
      icon: FileCheck,
      content: (
        <div className="space-y-6">
          <div className="bg-indigo-50 p-6 rounded-lg border-2 border-indigo-300">
            <h3 className="text-2xl font-bold mb-3 text-indigo-900">Your Digital "Handwritten Signature"</h3>
            <p className="text-lg text-gray-700">Proves a document came from you and hasn't been changed</p>
          </div>

          <div className="bg-white p-6 rounded-lg border-2 border-gray-300">
            <h3 className="text-xl font-bold mb-4 text-gray-800">How Alice Signs a Document:</h3>
            <div className="space-y-4">
              <div className="flex items-start gap-3">
                <span className="bg-indigo-600 text-white px-3 py-1 rounded font-bold min-w-[2rem] text-center">1</span>
                <div className="flex-1 bg-blue-50 p-3 rounded">
                  <p>Alice writes a document: "I agree to pay Bob $100"</p>
                </div>
              </div>
              
              <div className="flex items-start gap-3">
                <span className="bg-indigo-600 text-white px-3 py-1 rounded font-bold min-w-[2rem] text-center">2</span>
                <div className="flex-1 bg-purple-50 p-3 rounded">
                  <p>Alice creates a <strong>hash</strong> of the document (its fingerprint)</p>
                </div>
              </div>
              
              <div className="flex items-start gap-3">
                <span className="bg-indigo-600 text-white px-3 py-1 rounded font-bold min-w-[2rem] text-center">3</span>
                <div className="flex-1 bg-red-50 p-3 rounded">
                  <p>Alice encrypts the hash with her <strong>private key</strong> - this is the signature!</p>
                </div>
              </div>
              
              <div className="flex items-start gap-3">
                <span className="bg-indigo-600 text-white px-3 py-1 rounded font-bold min-w-[2rem] text-center">4</span>
                <div className="flex-1 bg-yellow-50 p-3 rounded">
                  <p>Alice sends both the document AND the signature to Bob</p>
                </div>
              </div>
              
              <div className="flex items-start gap-3">
                <span className="bg-indigo-600 text-white px-3 py-1 rounded font-bold min-w-[2rem] text-center">5</span>
                <div className="flex-1 bg-green-50 p-3 rounded">
                  <p>Bob uses Alice's <strong>public key</strong> to decrypt the signature and verify it matches the document's hash</p>
                </div>
              </div>
            </div>
          </div>

          <div className="grid grid-cols-3 gap-4">
            <div className="bg-green-100 p-4 rounded-lg text-center">
              <h4 className="font-bold text-green-900 mb-2">✅ Proves Identity</h4>
              <p className="text-sm text-gray-700">Only Alice has her private key, so it must be from her!</p>
            </div>
            <div className="bg-blue-100 p-4 rounded-lg text-center">
              <h4 className="font-bold text-blue-900 mb-2">✅ Detects Changes</h4>
              <p className="text-sm text-gray-700">If the document changes, the hash won't match!</p>
            </div>
            <div className="bg-purple-100 p-4 rounded-lg text-center">
              <h4 className="font-bold text-purple-900 mb-2">✅ Can't Deny</h4>
              <p className="text-sm text-gray-700">Alice can't later say "I didn't sign that!"</p>
            </div>
          </div>

          <div className="bg-yellow-100 p-4 rounded-lg border-2 border-yellow-400">
            <p className="font-bold text-gray-800">💡 Real Example: When you download software, the digital signature proves it really came from the company and wasn't hacked!</p>
          </div>
        </div>
      )
    },
    {
      title: "Public Key Infrastructure (PKI)",
      icon: Network,
      content: (
        <div className="space-y-6">
          <div className="bg-teal-50 p-6 rounded-lg border-2 border-teal-300">
            <h3 className="text-2xl font-bold mb-3 text-teal-900">The "Trust System" of the Internet</h3>
            <p className="text-lg text-gray-700">How do you know a website's public key actually belongs to them and not a hacker?</p>
          </div>

          <div className="bg-white p-6 rounded-lg border-2 border-gray-300">
            <h3 className="text-xl font-bold mb-4 text-gray-800">The Problem PKI Solves:</h3>
            <div className="bg-red-50 p-4 rounded-lg mb-4 border-l-4 border-red-500">
              <p className="text-gray-700">Without PKI: A hacker could pretend to be your bank, give you a fake public key, and steal your information! 😱</p>
            </div>
            <div className="bg-green-50 p-4 rounded-lg border-l-4 border-green-500">
              <p className="text-gray-700">With PKI: A trusted authority (like a notary) verifies identities and gives out certificates proving "this public key really belongs to Bank of America" ✅</p>
            </div>
          </div>

          <div className="bg-blue-50 p-6 rounded-lg">
            <h3 className="text-xl font-bold mb-4 text-blue-900">The Main Players:</h3>
            <div className="space-y-3">
              <div className="bg-white p-4 rounded shadow">
                <h4 className="font-bold text-gray-800 mb-2">🏛️ Certificate Authority (CA)</h4>
                <p className="text-gray-700">Like a government ID office - they check your identity and issue certificates. Examples: DigiCert, Let's Encrypt</p>
              </div>
              
              <div className="bg-white p-4 rounded shadow">
                <h4 className="font-bold text-gray-800 mb-2">📜 Digital Certificate</h4>
                <p className="text-gray-700">Like your driver's license - proves who you are. Contains your public key + your info + CA's signature</p>
              </div>
              
              <div className="bg-white p-4 rounded shadow">
                <h4 className="font-bold text-gray-800 mb-2">🔍 Your Browser</h4>
                <p className="text-gray-700">Checks certificates to make sure websites are legitimate. That's why you see a padlock 🔒 on secure sites!</p>
              </div>
            </div>
          </div>

          <div className="bg-purple-50 p-5 rounded-lg">
            <h4 className="font-bold text-purple-900 mb-3">When You Visit https://amazon.com:</h4>
            <div className="space-y-2 text-sm text-gray-700">
              <p>1️⃣ Amazon sends you their certificate (contains their public key)</p>
              <p>2️⃣ Your browser checks: "Is this certificate signed by a CA I trust?"</p>
              <p>3️⃣ If yes → Shows padlock 🔒, you're safe to shop!</p>
              <p>4️⃣ If no → Shows warning ⚠️, don't enter your credit card!</p>
            </div>
          </div>
        </div>
      )
    },
    {
      title: "Common Attacks (Know the Threats!)",
      icon: AlertTriangle,
      content: (
        <div className="space-y-6">
          <div className="bg-red-50 p-6 rounded-lg border-2 border-red-300">
            <div className="flex items-center gap-3 mb-3">
              <AlertTriangle className="w-8 h-8 text-red-900" />
              <h3 className="text-2xl font-bold text-red-900">How Hackers Try to Break Cryptography</h3>
            </div>
            <p className="text-lg text-gray-700">Understanding attacks helps you protect yourself!</p>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div className="bg-orange-50 p-5 rounded-lg border-2 border-orange-200">
              <h4 className="font-bold text-orange-900 mb-2 text-lg">🔨 Brute Force Attack</h4>
              <p className="text-gray-700 mb-2"><strong>What it is:</strong> Trying every possible password/key until finding the right one</p>
              <p className="text-sm text-gray-600"><strong>Example:</strong> Trying "0000", then "0001", then "0002"... for your phone PIN</p>
              <p className="text-sm text-green-700 mt-2"><strong>Defense:</strong> Use long, complex passwords!</p>
            </div>

            <div className="bg-yellow-50 p-5 rounded-lg border-2 border-yellow-200">
              <h4 className="font-bold text-yellow-900 mb-2 text-lg">🕵️ Man-in-the-Middle</h4>
              <p className="text-gray-700 mb-2"><strong>What it is:</strong> Hacker secretly sits between you and the website, reading everything</p>
              <p className="text-sm text-gray-600"><strong>Example:</strong> On public WiFi, hacker intercepts your messages to your bank</p>
              <p className="text-sm text-green-700 mt-2"><strong>Defense:</strong> Use HTTPS (padlock icon), avoid public WiFi for sensitive stuff</p>
            </div>

            <div className="bg-pink-50 p-5 rounded-lg border-2 border-pink-200">
              <h4 className="font-bold text-pink-900 mb-2 text-lg">🎣 Phishing</h4>
              <p className="text-gray-700 mb-2"><strong>What it is:</strong> Tricking you into giving away your password or key</p>
              <p className="text-sm text-gray-600"><strong>Example:</strong> Fake email from "your bank" asking you to click a link and enter your password</p>
              <p className="text-sm text-green-700 mt-2"><strong>Defense:</strong> Always check the sender, never click suspicious links</p>
            </div>

            <div className="bg-purple-50 p-5 rounded-lg border-2 border-purple-200">
              <h4 className="font-bold text-purple-900 mb-2 text-lg">⏱️ Side-Channel Attack</h4>
              <p className="text-gray-700 mb-2"><strong>What it is:</strong> Figuring out secrets by measuring time, power usage, or other side effects</p>
              <p className="text-sm text-gray-600"><strong>Example:</strong> Measuring how long it takes a system to decrypt different messages to guess the key</p>
              <p className="text-sm text-green-700 mt-2"><strong>Defense:</strong> Use well-designed software that prevents timing leaks</p>
            </div>

            <div className="bg-blue-50 p-5 rounded-lg border-2 border-blue-200">
              <h4 className="font-bold text-blue-900 mb-2 text-lg">🎂 Birthday Attack</h4>
              <p className="text-gray-700 mb-2"><strong>What it is:</strong> Finding two different inputs that produce the same hash (collision)</p>
              <p className="text-sm text-gray-600"><strong>Why "birthday":</strong> Like how in a room of 23 people, there's a 50% chance two share a birthday!</p>
              <p className="text-sm text-green-700 mt-2"><strong>Defense:</strong> Use strong hash algorithms like SHA-256</p>
            </div>

            <div className="bg-red-50 p-5 rounded-lg border-2 border-red-200">
              <h4 className="font-bold text-red-900 mb-2 text-lg">🔄 Replay Attack</h4>
              <p className="text-gray-700 mb-2"><strong>What it is:</strong> Recording a valid message and sending it again later</p>
              <p className="text-sm text-gray-600"><strong>Example:</strong> Hacker records your "transfer $100" command and replays it to steal more money</p>
              <p className="text-sm text-green-700 mt-2"><strong>Defense:</strong> Add timestamps or sequence numbers to messages</p>
            </div>
          </div>

          <div className="bg-green-50 p-5 rounded-lg border-2 border-green-400">
            <h4 className="font-bold text-green-900 mb-3 text-xl">🛡️ Your Best Defenses:</h4>
            <ul className="space-y-2 text-gray-700">
              <li>✅ Use strong, unique passwords for each account</li>
              <li>✅ Enable two-factor authentication (2FA)</li>
              <li>✅ Keep software updated</li>
              <li>✅ Look for HTTPS and the padlock 🔒</li>
              <li>✅ Be skeptical of unexpected emails or links</li>
              <li>✅ Use a password manager</li>
            </ul>
          </div>
        </div>
      )
    },
    {
      title: "Putting It All Together",
      icon: Shield,
      content: (
        <div className="space-y-6">
          <div className="bg-gradient-to-r from-blue-50 to-purple-50 p-6 rounded-lg border-2 border-blue-300">
            <h3 className="text-2xl font-bold mb-3 text-gray-800">Real-World Example: Shopping on Amazon</h3>
          </div>

          <div className="bg-white p-6 rounded-lg border-2 border-gray-300">
            <h4 className="font-bold text-gray-800 mb-4 text-xl">What Happens Behind the Scenes:</h4>
            <div className="space-y-4">
              <div className="bg-blue-50 p-4 rounded-lg border-l-4 border-blue-500">
                <p className="font-semibold text-blue-900 mb-2">Step 1: Secure Connection (Asymmetric + PKI)</p>
                <p className="text-gray-700">Your browser gets Amazon's certificate, verifies it with a CA, then uses Amazon's public key to establish a secure connection</p>
              </div>

              <div className="bg-green-50 p-4 rounded-lg border-l-4 border-green-500">
                <p className="font-semibold text-green-900 mb-2">Step 2: Fast Encryption (Symmetric)</p>
                <p className="text-gray-700">Your browser and Amazon agree on a symmetric key (using the asymmetric connection), then use it to quickly encrypt all your shopping data</p>
              </div>

              <div className="bg-purple-50 p-4 rounded-lg border-l-4 border-purple-500">
                <p className="font-semibold text-purple-900 mb-2">Step 3: Password Security (Hash)</p>
                <p className="text-gray-700">When you log in, Amazon compares the hash of your password to the stored hash (they never see your actual password!)</p>
              </div>

              <div className="bg-orange-50 p-4 rounded-lg border-l-4 border-orange-500">
                <p className="font-semibold text-orange-900 mb-2">Step 4: Integrity Check (Hash Again)</p>
                <p className="text-gray-700">Each message includes a hash to verify it wasn't tampered with in transit</p>
              </div>

              <div className="bg-pink-50 p-4 rounded-lg border-l-4 border-pink-500">
                <p className="font-semibold text-pink-900 mb-2">Step 5: Payment Security (Digital Signature)</p>
                <p className="text-gray-700">Your payment info is digitally signed to prove it came from you and prevent fraud</p>
              </div>
            </div>
          </div>

          <div className="bg-yellow-100 p-5 rounded-lg border-2 border-yellow-400">
            <p className="text-xl font-bold text-center text-gray-800">🎉 All three types of cryptography working together to keep you safe!</p>
          </div>
        </div>
      )
    },
    {
      title: "Quick Reference Guide",
      icon: BookOpen,
      content: (
        <div className="space-y-6">
          <h3 className="text-2xl font-bold text-center text-gray-800">Cryptography Cheat Sheet</h3>

          <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
            <div className="bg-purple-100 p-4 rounded-lg border-2 border-purple-300">
              <h4 className="font-bold text-purple-900 mb-3 text-center">Symmetric</h4>
              <div className="space-y-2 text-sm">
                <p><strong>Key:</strong> One shared key</p>
                <p><strong>Speed:</strong> ⚡ Very fast</p>
                <p><strong>Use:</strong> Encrypting large files</p>
                <p><strong>Example:</strong> AES</p>
                <p><strong>Analogy:</strong> House key 🔑</p>
              </div>
            </div>

            <div className="bg-green-100 p-4 rounded-lg border-2 border-green-300">
              <h4 className="font-bold text-green-900 mb-3 text-center">Asymmetric</h4>
              <div className="space-y-2 text-sm">
                <p><strong>Keys:</strong> Public + Private</p>
                <p><strong>Speed:</strong> 🐌 Slower</p>
                <p><strong>Use:</strong> Secure key exchange</p>
                <p><strong>Example:</strong> RSA</p>
                <p><strong>Analogy:</strong> Mailbox 📬</p>
              </div>
            </div>

            <div className="bg-orange-100 p-4 rounded-lg border-2 border-orange-300">
              <h4 className="font-bold text-orange-900 mb-3 text-center">Hash</h4>
              <div className="space-y-2 text-sm">
                <p><strong>Keys:</strong> None needed</p>
                <p><strong>Direction:</strong> ↓ One-way only</p>
                <p><strong>Use:</strong> Verify integrity</p>
                <p><strong>Example:</strong> SHA-256</p>
                <p><strong>Analogy:</strong> Blender 🥤</p>
              </div>
            </div>
          </div>

          <div className="bg-blue-50 p-5 rounded-lg">
            <h4 className="font-bold text-blue-900 mb-3 text-lg">The Four Goals:</h4>
            <div className="grid grid-cols-2 gap-3 text-sm">
              <p>🔒 <strong>Confidentiality:</strong> Keep secrets secret</p>
              <p>✅ <strong>Integrity:</strong> Detect tampering</p>
              <p>👤 <strong>Authentication:</strong> Verify identity</p>
              <p>📝 <strong>Non-repudiation:</strong> Can't deny it</p>
            </div>
          </div>

          <div className="bg-gradient-to-r from-green-100 to-blue-100 p-5 rounded-lg">
            <h4 className="font-bold text-gray-800 mb-3 text-lg">Where You See Cryptography Daily:</h4>
            <div className="grid grid-cols-2 gap-2 text-sm text-gray-700">
              <p>🔒 HTTPS websites</p>
              <p>💬 WhatsApp messages</p>
              <p>🏦 Online banking</p>
              <p>📧 Email encryption</p>
              <p>📱 Phone unlock</p>
              <p>💳 Credit card payments</p>
              <p>☁️ Cloud storage</p>
              <p>🎮 Gaming accounts</p>
            </div>
          </div>
        </div>
      )
    },
    {
      title: "Key Takeaways",
      icon: Shield,
      content: (
        <div className="space-y-6">
          <div className="text-center space-y-4">
            <Shield className="w-32 h-32 mx-auto text-blue-600" />
            <h3 className="text-3xl font-bold text-gray-800">What You've Learned! 🎓</h3>
          </div>

          <div className="space-y-4">
            <div className="bg-blue-50 p-5 rounded-lg border-l-4 border-blue-600">
              <p className="text-lg text-gray-800"><strong>Cryptography</strong> is how we keep information safe in the digital world</p>
            </div>

            <div className="bg-purple-50 p-5 rounded-lg border-l-4 border-purple-600">
              <p className="text-lg text-gray-800"><strong>Three main types:</strong> Symmetric (same key), Asymmetric (two keys), and Hash functions (one-way)</p>
            </div>

            <div className="bg-green-50 p-5 rounded-lg border-l-4 border-green-600">
              <p className="text-lg text-gray-800"><strong>Digital signatures</strong> prove who sent a message and that it wasn't changed</p>
            </div>

            <div className="bg-orange-50 p-5 rounded-lg border-l-4 border-orange-600">
              <p className="text-lg text-gray-800"><strong>PKI</strong> creates a trust system so you know you're talking to the right person/website</p>
            </div>

            <div className="bg-red-50 p-5 rounded-lg border-l-4 border-red-600">
              <p className="text-lg text-gray-800"><strong>Attacks are real</strong> - but understanding them helps you stay protected</p>
            </div>

            <div className="bg-yellow-50 p-5 rounded-lg border-l-4 border-yellow-600">
              <p className="text-lg text-gray-800"><strong>You use cryptography every day</strong> - every time you see that padlock 🔒!</p>
            </div>
          </div>

          <div className="bg-gradient-to-r from-blue-600 to-purple-600 text-white p-6 rounded-lg text-center">
            <p className="text-2xl font-bold mb-2">Cryptography: Your Digital Guardian! 🛡️</p>
            <p className="text-lg">Protecting your privacy, one encrypted bit at a time</p>
          </div>

          <div className="bg-gray-100 p-5 rounded-lg text-center">
            <p className="text-gray-700"><strong>Remember:</strong> Strong passwords + 2FA + updated software = A safer you! ✨</p>
          </div>
        </div>
      )
    }