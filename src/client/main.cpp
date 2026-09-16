#include "Client.h"

int main(int argc, char const* const* argv)
{
  Client client;
  client.connect(argc, argv);
  return 0;
}